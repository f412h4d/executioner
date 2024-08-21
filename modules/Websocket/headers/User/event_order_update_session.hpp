#ifndef EVENT_ORDER_UPDATE_SESSION_HPP
#define EVENT_ORDER_UPDATE_SESSION_HPP

#include "../Market/price_settings.hpp"
#include "order_model.h"
#include "spdlog/spdlog.h"
#include "trade_signal.h"
#include "order.h"
#include "user_session.hpp"
#include <nlohmann/json.hpp>

extern std::shared_ptr<Order> order;
extern std::mutex order_mutex;

class event_order_update_session : public user_session {
public:
  event_order_update_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams &api_params,
                             std::shared_ptr<PriceSettings> price_settings, std::mutex &mutex)
      : user_session(ioc, ctx, api_params), price_settings_(price_settings), price_mutex_(mutex) {}

  void run(const std::string &host, const std::string &port) override {
    listen_key_ = get_listen_key();
    if (listen_key_.empty()) {
      auto account_logger = spdlog::get("account_logger");
      account_logger->error("Failed to get listenKey");
      return;
    }
    user_session::run(host, port);
  }

  void on_handshake(boost::system::error_code ec) override {
    auto account_logger = spdlog::get("account_logger");
    if (ec)
      return fail(ec, "handshake");

    account_logger->info("Event Order Update: Handshake completed");
    send_subscribe_message();
  }

  void send_subscribe_message() override {
    nlohmann::json j;
    j["id"] = "order-update-id";
    j["method"] = "SUBSCRIBE";
    j["params"] = {"btcusdt@orderUpdate"};

    ws_.async_write(boost::asio::buffer(j.dump()), [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
    auto account_logger = spdlog::get("account_logger");
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "read");

    std::string message = boost::beast::buffers_to_string(buffer_.data());
    account_logger->info("Account Update Message: {}", message);

    try {
      // Parse the message as JSON
      nlohmann::json json_message = nlohmann::json::parse(message);

      // Extract the nested order information
      if (json_message.contains("o")) {
        const auto &order_info = json_message["o"];

        // Check if the order has been canceled
        if (order_info.contains("x") && order_info["x"] == "CANCELED" && order_info.contains("X") &&
            order_info["X"] == "CANCELED") {

          std::string clientOrderId = order_info["c"];
          std::string orderId = std::to_string(order_info["i"].get<long>());

          account_logger->info("Order {} with Client Order ID {} has been canceled.", orderId, clientOrderId);

          {
            std::lock_guard<std::mutex> lock(order_mutex);
            if (order->order_id == orderId) {
              order->status = "CANCELED";
              account_logger->info("Updated shared order status to CANCELED.");
            } else {
              account_logger->warn("Order ID {} does not match the shared order ID.", orderId);
            }
          }
        }
        else if (order_info.contains("x") && order_info["x"] == "TRADE" && order_info.contains("X") &&
                 order_info["X"] == "FILLED" && order_info.contains("o") && order_info["o"] == "LIMIT") {

          std::string clientOrderId = order_info["c"];
          std::string orderId = std::to_string(order_info["i"].get<long>());

          account_logger->info("LIMIT Order {} with Client Order ID {} has been filled.", orderId, clientOrderId);

          double quantity = std::round(std::stod(order_info["z"].get<std::string>()) * 1000) / 1000;
          double tpPrice = std::round(price_settings_->calculated_tp * 100) / 100;
          double slPrice = std::round(price_settings_->calculated_sl * 100) / 100;

          std::string side = order_info["S"].get<std::string>();
          std::string inverted_side = (side == "BUY") ? "SELL" : "BUY";

          account_logger->info("Placing TP and SL orders for filled LIMIT order: {}", orderId);

          SignalService::placeTpAndSlOrders(api_params_, inverted_side, quantity, tpPrice, slPrice);
        }
        // Check if the order has been filled and is a MARKET order
        else if (order_info.contains("x") && order_info["x"] == "TRADE" && order_info.contains("X") &&
                 order_info["X"] == "FILLED" && order_info.contains("o") && order_info["o"] == "MARKET") {

          std::string clientOrderId = order_info["c"];
          std::string orderId = std::to_string(order_info["i"].get<long>());

          account_logger->info("MARKET Order {} with Client Order ID {} has been filled.", orderId, clientOrderId);

          // Cleanup by canceling all remaining orders
          account_logger->info("Initiating cleanup by canceling all open orders.");

          auto response = OrderService::cancelAllOpenOrders(api_params_, "BTCUSDT");

          account_logger->info("Cancel All Orders Response: {}", response.dump(4));
        } else {
          account_logger->info("Order update received but the status or type is not as expected.");
        }
      } else {
        account_logger->warn("Received message does not contain order information.");
      }
    } catch (const std::exception &e) {
      account_logger->error("Failed to parse or process order update: {}", e.what());
    }

    // Clear the buffer after processing
    buffer_.consume(buffer_.size());

    // Continue reading from the WebSocket
    ws_.async_read(buffer_, [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

private:
  std::shared_ptr<PriceSettings> price_settings_;
  std::mutex &price_mutex_;
};

#endif // EVENT_ORDER_UPDATE_SESSION_HPP
