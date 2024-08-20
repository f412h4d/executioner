#include <csignal>
#include <cstdlib> // For setenv
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "APIParams.h"
#include "logger.h"
#include "margin.h"
#include "modules/TradeSignal/headers/trade_signal.h"
#include "modules/TradeSignal/models/signal_settings_model.h"
#include "modules/TradeSignal/models/trade_signal_model.h"
#include "utils.h"

// Websocket
#include "modules/Websocket/headers/Market/price_session.hpp"
#include "modules/Websocket/headers/Market/price_settings.hpp"
#include "modules/Websocket/headers/User/event_order_update_session.hpp"
#include "modules/Websocket/headers/root_certificates.hpp"
// Websocket Boost libs
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

// Google cloud pubsub
#include "google/cloud/pubsub/subscriber.h"
#include "google/cloud/pubsub/subscriber_connection.h"

#define LEVERAGE 1

namespace pubsub = google::cloud::pubsub;

// Shared object and mutex between websocket threads

// Shared object for signal settings
auto signal_settings = std::make_shared<SignalSettings>();

auto price_settings = std::make_shared<PriceSettings>();
std::mutex price_mutex;

// Global variable to store the subscriber's start time
auto subscriber_start_time = std::chrono::steady_clock::now();

void pubsub_callback(pubsub::Message const &message, pubsub::AckHandler ack_handler, const APIParams &apiParams) {
  auto pubsub_logger = spdlog::get("pubsub_logger");
  auto signal_logger = spdlog::get("signal_logger");
  auto news_logger = spdlog::get("news_logger");
  auto is_active_logger = spdlog::get("is_active_logger");

  pubsub_logger->info("Received message: {}", message.data());

  // Check if the message was received within the first few seconds after startup
  auto now = std::chrono::steady_clock::now();
  auto time_since_start = std::chrono::duration_cast<std::chrono::seconds>(now - subscriber_start_time).count();

  // Discard messages received within the first 5 seconds
  if (time_since_start <= 5) {
    pubsub_logger->info("Discarding message received within the first 5 seconds after startup.");
    std::move(ack_handler).ack(); // Acknowledge the message to prevent reprocessing
    return;
  }

  try {
    // Parse the message JSON to extract the "type"
    auto json_message = nlohmann::json::parse(message.data());
    std::string message_type = json_message["type"];

    if (message_type == "signal") {
      std::lock_guard<std::mutex> lock(signal_settings->signal_mutex);
      TradeSignal signal;
      signal.fromJsonString(message.data());
      signal_settings->signal = signal;

      signal_logger->info("Received signal: {}", signal_settings->signal.toJsonString());

      auto balance_response = Margin::getBalance(apiParams, "BTCUSDT");
      double balance = balance_response["availableBalance"].get<double>();
      double quantity = std::floor((balance * LEVERAGE / price_settings->calculated_price) * 1000) / 1000;

      SignalService::process(signal_settings->signal.value, apiParams, quantity, price_settings->calculated_price);
    } else if (message_type == "news") {
      std::lock_guard<std::mutex> lock(signal_settings->news_range_mutex);
      signal_settings->news_range = DatetimeRange::fromJsonString(message.data());
      news_logger->info("Received news message: {}", signal_settings->news_range.toJsonString());
    } else if (message_type == "deactivate") {
      std::lock_guard<std::mutex> lock(signal_settings->deactivate_range_mutex);
      signal_settings->deactivate_range = DatetimeRange::fromJsonString(message.data());
      is_active_logger->info("Received deactivate message: {}", signal_settings->deactivate_range.toJsonString());
    } else {
      pubsub_logger->warn("Unknown message type: {}", message_type);
    }
  } catch (const nlohmann::json::exception &e) {
    // Handle JSON parsing errors or other JSON-related issues
    pubsub_logger->error("JSON parsing error: {}", e.what());
    pubsub_logger->error("Message data: {}", message.data());

    // Optionally, decide how to handle this case (e.g., skip the message)
    std::move(ack_handler).ack(); // Acknowledge the message to prevent reprocessing
    return;
  } catch (const std::exception &e) {
    // Handle other exceptions that may occur
    pubsub_logger->error("Error processing message: {}", e.what());
    pubsub_logger->error("Message data: {}", message.data());

    // Optionally, decide how to handle this case (e.g., skip the message)
    std::move(ack_handler).ack(); // Acknowledge the message to prevent reprocessing
    return;
  }

  // If everything goes well, acknowledge the message
  std::move(ack_handler).ack();
}

int main() {
  Logger::setup_loggers();
  auto pubsub_logger = spdlog::get("pubsub_logger");
  auto market_logger = spdlog::get("market_logger");
  auto account_logger = spdlog::get("account_logger");

  // Setting up Binance API keys
  std::string exePath = Utils::getExecutablePath();
  std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
  std::string envFilePath = exeDir + "/../.env";
  std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

  bool useTestnet = (env["TESTNET"] == "TRUE");
  std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
  std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

  APIParams apiParams(apiKey, apiSecret, 5000, useTestnet);

  // Setting up Google cloud pubsub
  const char *credentials_path = env["GOOGLE_CLOUD_CREDENTIALS_PATH"].c_str();
  setenv("GOOGLE_APPLICATION_CREDENTIALS", credentials_path, 1);

  std::string const project_id = env["GOOGLE_CLOUD_PROJECT_ID"];
  std::string const subscription_id = env["GOOGLE_CLOUD_SUBSCRIPTION_ID"];

  auto subscription = pubsub::Subscription(project_id, subscription_id);
  auto subscriber_connection = pubsub::MakeSubscriberConnection(subscription);
  auto subscriber = pubsub::Subscriber(subscriber_connection);

  // Setting up threads to run subscription async
  std::vector<std::thread> threads;

  // Setting up websocket context
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto ctx = std::make_shared<ssl::context>(ssl::context::sslv23_client);
  load_root_certificates(*ctx);

  threads.emplace_back([&, apiParams]() {
    pubsub_logger->info("Listening for messages on Google subs: {}", subscription.FullName());
    subscriber
        .Subscribe([&apiParams](pubsub::Message const &message, pubsub::AckHandler ack_handler) {
          pubsub_callback(message, std::move(ack_handler), apiParams);
        })
        .get();
  });

  // auto balance_session_instance = std::make_shared<balance_session>(*ioc, *ctx, apiParams, balance, balance_mutex);
  // threads.emplace_back([&, ioc]() {
  //   account_logger->info("Listening for balance updates");
  //   balance_session_instance->run("fstream.binance.com", "443");
  //   ioc->run();
  // });
  //
  // auto account_status_session_instance =
  //     std::make_shared<margin_session>(*ioc, *ctx, apiParams, account_status, account_status_mutex);
  // threads.emplace_back([&, ioc]() {
  //   account_logger->info("Listening for account status updates");
  //   account_status_session_instance->run("fstream.binance.com", "443");
  //   ioc->run();
  // });

  // auto price_session_instance = std::make_shared<price_session>(*ioc, *ctx, price_settings, price_mutex);
  // threads.emplace_back([&, ioc]() {
  //   market_logger->info("Listening for messages on Binance price ticket: ");
  //   price_session_instance->run("fstream.binance.com", "443");
  //   ioc->run();
  // });
  //
  // auto event_order_session =
  //     std::make_shared<event_order_update_session>(*ioc, *ctx, apiParams, price_settings, price_mutex);
  // threads.emplace_back([&, ioc]() {
  //   account_logger->info("Listening for messages on Binance user stream: ");
  //   event_order_session->run("fstream.binance.com", "443");
  //   event_order_session->start_keep_alive();
  //   ioc->run();
  // });

  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  return EXIT_SUCCESS;
}
