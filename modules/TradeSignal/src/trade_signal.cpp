#include "trade_signal.h"
#include "../../../modules/TradeSignal/models/trade_signal_model.h"
#include "margin.h"
#include "order.h"
#include "order_model.h"
#include "side.h"
#include "spdlog/spdlog.h"
#include <csignal>

#define CANCEL_DELAY 10 // Open Order Elimination
#define TICK_SIZE 0.1
#define SYMBOL "BTCUSDT"

extern std::shared_ptr<Order> order;
extern std::mutex order_mutex;

namespace SignalService {

bool prepareForOrder(const APIParams &apiParams) {
  std::string notional;
  size_t array_length;
  auto signal_logger = spdlog::get("signal_logger");
  auto margin_logger = spdlog::get("margin_logger");

  auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
  if (positions_response.is_array() && positions_response[0].contains("notional")) {
    notional = positions_response[0]["notional"].get<std::string>();
    margin_logger->debug("Notional is fetched: {}", notional);
  } else {
    margin_logger->error("Notional not found in the response");
    margin_logger->info("Signal aborted");
    return false;
  }

  auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");

  if (open_orders_response.is_array()) {
    array_length = open_orders_response.size();
    margin_logger->debug("Open orders size fetched & calculated: {}", array_length);
  } else {
    margin_logger->error("Unexpected response format: {}", open_orders_response.dump(4));
    margin_logger->info("Signal aborted");
    return false;
  }

  if (notional != "0") {
    margin_logger->info("Notional is not 0");
    margin_logger->info("Signal aborted");
    return false;
  }

  if (array_length < 1) {
    margin_logger->info("Signal accepted");
    return true;
  }

  for (const auto &order : open_orders_response) {
    if (order.contains("origType") && order["origType"] == "LIMIT") {
      margin_logger->info("Detected LIMIT order in open orders list");
      margin_logger->info("Signal aborted");
      return false;
    }
  }

  margin_logger->info("Running Cancel all for pre-execution clean up");
  auto response = OrderService::cancelAllOpenOrders(apiParams, SYMBOL);
  margin_logger->debug("Canceled all orders: {}", response.dump(4));
  return true;
}

void process(int signal, const APIParams &apiParams, double quantity, double entryPrice) {
  auto exec_logger = spdlog::get("exec_logger");
  if (signal == 0) {
    exec_logger->warn("Ignoring the 0 signal");
    return;
  }

  bool validConditions = prepareForOrder(apiParams);
  if (!validConditions) {
    return;
  }

  OrderInput order_input(SYMBOL, Side::fromSignal(signal), "LIMIT", "GTC", quantity, entryPrice);
  auto order_response = OrderService::createOrder(apiParams, order_input);

  if (order_response.empty()) {
    exec_logger->critical("Order response is empty.");
    return;
  }

  try {
    auto [order_id, side, status, quantity, price] = OrderService::validateOrderResponse(order_response);
    {
      std::lock_guard<std::mutex> lock(order_mutex);
      order->order_id = order_id;
      order->side = side;
      order->status = status;
      order->quantity = quantity;
      order->price = price;
    }
  } catch (const std::exception &e) {
    exec_logger->critical("Order validation failed: {}", e.what());
    return;
  }

  exec_logger->info("Order placed successfully: {}", order_response.dump(4));
}

void cancelWithDelay(TradeSignal signal, const APIParams &apiParams, SignalQueue &cancelQueue) {
  auto cancel_queue_logger = spdlog::get("cancel_queue_logger");
  cancel_queue_logger->info("Signal: {} Added to queue to be canceled", signal.toJsonString());

  cancelQueue.addEvent(
      TIME::now() + std::chrono::seconds(CANCEL_DELAY), "Order Cancel", [apiParams, cancel_queue_logger]() {
        std::string order_id;
        {
          std::lock_guard<std::mutex> lock(order_mutex);
          order_id = order->order_id;
        }
        auto order_details = OrderService::getOrderDetails(apiParams, "BTCUSDT", order_id, "");

        if (!order_details.contains("status")) {
          cancel_queue_logger->error("Failed to fetch order status. Response: {}", order_details.dump(4));
          return;
        }

        std::string status = order_details["status"].get<std::string>();
        if (status != "NEW") {
          cancel_queue_logger->warn("Unexpected order status '{}', aborting cancel. Order Id: {}", status, order_id);
          return;
        }

        OrderService::cancelOrder(apiParams, "BTCUSDT", order_id, "");
        // ? Order status will be updated by websocket
      });
}

void placeTpAndSlOrders(const APIParams &apiParams, std::string side, double quantity, double tpPrice, double slPrice) {
  auto exec_logger = spdlog::get("exec_logger");
  std::vector<std::thread> threads;

  threads.emplace_back([&]() {
    TriggerOrderInput tpOrder(SYMBOL, side, "TAKE_PROFIT_MARKET", "GTC", quantity, tpPrice, tpPrice, true);
    auto tp_response = OrderService::createTriggerOrder(apiParams, tpOrder);
    exec_logger->debug("TP Order Response: {}", tp_response.dump(4));
  });

  threads.emplace_back([&]() {
    TriggerOrderInput slOrder(SYMBOL, side, "STOP_MARKET", "GTC", quantity, slPrice, slPrice, true);
    auto sl_response = OrderService::createTriggerOrder(apiParams, slOrder);
    exec_logger->debug("SL Order Response: {}", sl_response.dump(4));
  });

  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

} // namespace SignalService
