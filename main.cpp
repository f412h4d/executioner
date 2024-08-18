#include <cstdlib> // For setenv
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "APIParams.h"
#include "logger.h"
#include "modules/TradeSignal/headers/trade_signal.h"
#include "modules/TradeSignal/models/trade_signal_model.h"
#include "utils.h"

#include "modules/Margin/headers/account_status_model.h"

// Websocket
#include "modules/Websocket/headers/Market/price_session.hpp"
#include "modules/Websocket/headers/Market/price_settings.hpp"
#include "modules/Websocket/headers/User/balance_session.hpp"
#include "modules/Websocket/headers/User/event_order_update_session.hpp"
#include "modules/Websocket/headers/User/margin_session.hpp"
#include "modules/Websocket/headers/root_certificates.hpp"
// Websocket Boost libs
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

// Google cloud pubsub
#include "google/cloud/pubsub/subscriber.h"
#include "google/cloud/pubsub/subscriber_connection.h"

namespace pubsub = google::cloud::pubsub;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using ssl_stream = boost::asio::ssl::stream<tcp::socket>;

void websocket_request(const APIParams &apiParams) {
  try {
    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    websocket::stream<ssl_stream> ws{ioc, ctx};

    // Resolve the host
    auto const results = resolver.resolve("ws-fapi.binance.com", "443");

    // Make the connection on the IP address we get from a lookup
    net::connect(ws.next_layer().next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);

    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
      req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-cxx");
    }));

    // Perform the WebSocket handshake
    std::string host = "ws-fapi.binance.com";
    ws.handshake(host, "/ws-fapi/v1");

    // Prepare the request
    long timestamp = time(NULL) * 1000; // UNIX timestamp in milliseconds

    // Prepare the data string used for HMAC
    std::string data = "apiKey=" + apiParams.apiKey + "&timestamp=" + std::to_string(timestamp);

    // Generate the signature using Ed25519
    std::string signature = Utils::generate_ed25519_signature(data, apiParams.apiSecret);

    // Create the JSON request
    std::string message = R"({
        "id": "unique_request_id",
        "method": "session.logon",
        "params": {
            "apiKey": ")" +
                          apiParams.apiKey + R"(",
            "timestamp": )" +
                          std::to_string(timestamp) + R"(,
            "signature": ")" +
                          signature + R"("
        }
    })";

    // Send the message
    ws.write(net::buffer(std::string(message)));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    ws.read(buffer);

    // Close the WebSocket connection
    ws.close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully

    // The message is received, convert it to a string and print
    std::cout << beast::make_printable(buffer.data()) << std::endl;
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

void pubsub_callback(pubsub::Message const &message, pubsub::AckHandler ack_handler) {
  auto pubsub_logger = spdlog::get("pubsub_logger");
  auto signal_logger = spdlog::get("signal_logger");
  pubsub_logger->info("Received message: {}", message.data());

  TradeSignal signal = TradeSignal::fromJsonString(message.data());
  signal_logger->info("Received signal: {}", signal.toJsonString());

  std::move(ack_handler).ack();
}

// Shared object and mutex between websocket threads
auto price_settings = std::make_shared<PriceSettings>();
std::mutex price_mutex;

auto balance = std::make_shared<Balance>();
std::mutex balance_mutex;

auto account_status = std::make_shared<AccountStatus>();
std::mutex account_status_mutex;

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

  websocket_request(apiParams);

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

  // threads.emplace_back([&]() {
  //   pubsub_logger->info("Listening for messages on Google subs: {}", subscription.FullName());
  //   subscriber.Subscribe(pubsub_callback).get();
  // });
  //
  // auto balance_session_instance = std::make_shared<balance_session>(*ioc, *ctx, apiParams, balance, balance_mutex);
  // threads.emplace_back([&, ioc]() {
  //   account_logger->info("Listening for balance updates");
  //   balance_session_instance->run("fstream.binance.com", "443");
  //   ioc->run();
  // });
  //
  // auto account_status_session_instance = std::make_shared<margin_session>(*ioc, *ctx, apiParams);
  // threads.emplace_back([&, ioc]() {
  //   account_logger->info("Listening for account status updates");
  //   account_status_session_instance->run("testnet.binancefuture.com", "443");
  //   ioc->run();
  // });
  //
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
