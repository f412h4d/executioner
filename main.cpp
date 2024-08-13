#include <cstdlib> // For setenv
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "APIParams.h"
#include "logger.h"
#include "utils.h"

// Websocket
#include "modules/Websocket/headers/Market/price_session.hpp"
#include "modules/Websocket/headers/User/event_order_update_session.hpp"
#include "modules/Websocket/headers/root_certificates.hpp"
// Websocket Boost libs
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

// Google cloud pubsub
#include "google/cloud/pubsub/subscriber.h"
#include "google/cloud/pubsub/subscriber_connection.h"

namespace pubsub = google::cloud::pubsub;

void pubsub_callback(pubsub::Message const &message, pubsub::AckHandler ack_handler) {
  Logger::pubsub_logger->info("Received message: ", message.data());
  std::move(ack_handler).ack();
}

// shared values between websocket threads
auto calculated_price = std::make_shared<double>(0.0);
std::mutex price_mutex;

int main() {
  // setting up Binance API keys
  std::string exePath = Utils::getExecutablePath();
  std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
  std::string envFilePath = exeDir + "/../.env";
  std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

  bool useTestnet = (env["TESTNET"] == "TRUE");
  std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
  std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

  APIParams apiParams(apiKey, apiSecret, 5000, useTestnet);

  // setting up Google cloud pubsub
  const char *credentials_path = env["GOOGLE_CLOUD_CREDENTIALS_PATH"].c_str();
  setenv("GOOGLE_APPLICATION_CREDENTIALS", credentials_path, 1);

  std::string const project_id = env["GOOGLE_CLOUD_PROJECT_ID"];
  std::string const subscription_id = env["GOOGLE_CLOUD_SUBSCRIPTION_ID"];

  auto subscription = pubsub::Subscription(project_id, subscription_id);
  auto subscriber_connection = pubsub::MakeSubscriberConnection(subscription);
  auto subscriber = pubsub::Subscriber(subscriber_connection);

  // setting up threads to run subscription async
  std::vector<std::thread> threads;

  // setting up websocket context
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto ctx = std::make_shared<ssl::context>(ssl::context::sslv23_client);
  load_root_certificates(*ctx);

  threads.emplace_back([&]() {
    Logger::pubsub_logger->info("Listening for messages on Google subs: ", subscription.FullName());
    subscriber.Subscribe(pubsub_callback).get();
  });

  auto price_session_instance = std::make_shared<price_session>(*ioc, *ctx, calculated_price, price_mutex);
  threads.emplace_back([&, ioc]() {
    Logger::pubsub_logger->info("Listening for messages on Binance price ticket: ");
    price_session_instance->run("fstream.binance.com", "443");
    ioc->run();
  });

  auto event_order_session =
      std::make_shared<event_order_update_session>(*ioc, *ctx, apiParams, calculated_price, price_mutex);
  threads.emplace_back([&, ioc]() {
    Logger::pubsub_logger->info("Listening for messages on Binance user stream: ");
    event_order_session->run("fstream.binance.com", "443");
    event_order_session->start_keep_alive();
    ioc->run();
  });

  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  return EXIT_SUCCESS;
}
