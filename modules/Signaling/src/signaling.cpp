#include "signaling.h"
#include "../../TimedEventQueue/headers/SignalQueue.h"
#include "logger.h"
#include "margin.h"
#include "news.h"
#include "order.h"
#include "utils.h"

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#define EXEC_DELAY 1      // Entry Time offset
#define CANCEL_DELAY 3001 // Open Order Elimination
#define MONITOR_DELAY 1
#define CALC_PRICE_PERCENTAGE (-0.0008) // Entry Gap needs to be minus
#define TP_PRICE_PERCENTAGE 0.014
#define SL_PRICE_PERCENTAGE (-0.01)

#define TICK_SIZE 0.1

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

  margin_logger->info("Running Cancel all for pre-executaion clean up");
  auto response = OrderService::cancelAllOpenOrders(apiParams, "BTCUSDT");
  margin_logger->debug("Canceled all orders: {}", response.dump(4));
  return true;
}

void placeTpAndSlOrders(const APIParams &apiParams, const std::string &symbol, const std::string &side, double orig_qty,
                        double tpPrice, double slPrice) {
  int signal = side == "SELL" ? 1 : -1;
  auto price = Margin::getPrice(apiParams, "BTCUSDT");
  double calculated_price = Utils::roundToTickSize(price * (1 + (CALC_PRICE_PERCENTAGE * signal)), TICK_SIZE);
  double newTpPrice = Utils::roundToTickSize(calculated_price * (1 + (TP_PRICE_PERCENTAGE * signal)), TICK_SIZE);
  double newSlPrice = Utils::roundToTickSize(calculated_price * (1 + (SL_PRICE_PERCENTAGE * signal)), TICK_SIZE);

  TriggerOrderInput tpOrder(symbol, side, "TAKE_PROFIT_MARKET", "GTC", orig_qty, newTpPrice, newTpPrice, true);
  auto tp_response = OrderService::createTriggerOrder(apiParams, tpOrder);
  std::cout << "TP Order Response: " << tp_response.dump(4) << std::endl;

  TriggerOrderInput slOrder(symbol, side, "STOP_MARKET", "GTC", orig_qty, newSlPrice, newSlPrice, true);
  auto sl_response = OrderService::createTriggerOrder(apiParams, slOrder);
  std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
}

bool isOrderFilled(const APIParams &apiParams) {
  std::string notional;

  auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
  if (positions_response.is_array() && positions_response[0].contains("notional")) {
    notional = positions_response[0]["notional"].get<std::string>();
  } else {
    std::cerr << "Notional not found in the response" << std::endl;
    return false;
  }

  return notional != "0";
}

void monitorOrderAndPlaceTpSl(SignalQueue &signalQueue, const APIParams &apiParams, const std::string &symbol,
                              const std::string &side, std::string &order_id, double &orig_qty, double &tpPrice,
                              double &slPrice, bool &monitor_lock) {
  std::cout << "Monitor Order Status will run in 1 secs.\n";
  signalQueue.addEvent(TIME::now() + std::chrono::seconds(MONITOR_DELAY), "Monitor Order Status",
                       [&apiParams, &symbol, &side, &order_id, &orig_qty, &tpPrice, &slPrice, &monitor_lock]() {
                         if (monitor_lock) {
                           std::cout << "Monitoring is locked, waiting for the order to be executed.\n";
                           return;
                         }

                         std::string order_status = "none";
                         auto response = OrderService::getOrderDetails(apiParams, "BTCUSDT", order_id);
                         if (response["status"].is_string()) {
                           order_status = response["status"].get<std::string>();
                         }
                         if (order_status == "CANCELED") {
                           std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"
                                     << "Order Is CANCELED Aborting TP & SL\n";
                           monitor_lock = true;
                           return;
                         }

                         if (isOrderFilled(apiParams)) {
                           std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n"
                                     << "Order Is FILLED Adding TP & SL\n";
                           monitor_lock = true;
                           placeTpAndSlOrders(apiParams, symbol, side, orig_qty, tpPrice, slPrice);
                         } else {
                           std::cout << "Not filled yet, will check again later.\n";
                         }
                       });
}

void cancelWithDelay(int signal, const APIParams &apiParams, SignalQueue &signalQueue, SignalQueue &tpSlQueue,
                     bool &lock) {
  std::cout << "Signal #" + std::to_string(signal) + " Added to queue to be canceled" << std::endl;
  signalQueue.addEvent(TIME::now() + std::chrono::seconds(CANCEL_DELAY),
                       "Trying to cancel the order " + std::to_string(signal), [&apiParams, &tpSlQueue, &lock]() {
                         std::string notional;
                         auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
                         if (positions_response.is_array() && positions_response[0].contains("notional")) {
                           notional = positions_response[0]["notional"];
                         } else {
                           std::cerr << "Notional not found in the response" << std::endl;
                           return;
                         }

                         if (notional != "0") {
                           std::cout << "Canceling aborted due to open position\n";
                           return;
                         }

                         std::cout << "\nCanceling:\n";
                         auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
                         if (open_orders_response.is_array() && !open_orders_response.empty()) {
                           auto response = OrderService::cancelAllOpenOrders(apiParams, "BTCUSDT");
                           std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;

                           lock = true;
                         } else {
                           std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
                         }
                       });
}

void processSignal(int signal, const APIParams &apiParams, SignalQueue &signalQueue, const std::string &side,
                   std::string &last_order_id, double &last_orig_qty, double &last_tp_price, double &last_sl_price,
                   bool &monitor_lock) {
  std::cout << "Signaling received: " << side << std::endl;
  std::cout << "Signal " << signal << " is going to be executed in " + std::to_string(EXEC_DELAY) + " seconds"
            << std::endl;

  signalQueue.addEvent(
      TIME::now() + std::chrono::seconds(EXEC_DELAY), "Signal is executed.",
      [&apiParams, signal, side, &last_order_id, &last_orig_qty, &last_tp_price, &last_sl_price, &monitor_lock]() {
        bool validConditions = prepareForOrder(apiParams);
        if (!validConditions) {
          return;
        }

        auto price = Margin::getPrice(apiParams, "BTCUSDT");
        double orig_price = price * (1 + (CALC_PRICE_PERCENTAGE * signal));
        double calculated_price = Utils::roundToTickSize(orig_price, TICK_SIZE);
        double tpPrice = Utils::roundToTickSize(calculated_price * (1 + (TP_PRICE_PERCENTAGE * signal)), TICK_SIZE);
        double slPrice = Utils::roundToTickSize(calculated_price * (1 + (SL_PRICE_PERCENTAGE * signal)), TICK_SIZE);

        OrderInput order("BTCUSDT", side, "LIMIT", "GTC", 0.005, calculated_price);

        auto order_response = OrderService::createOrder(apiParams, order);
        std::cout << "Order Response: " << order_response.dump(4) << std::endl;

        if (order_response.contains("orderId")) {
          double orig_qty = 0.0;
          std::string orderId;

          if (order_response["origQty"].is_string()) {
            orig_qty = std::stod(order_response["origQty"].get<std::string>());
          } else if (order_response["origQty"].is_number()) {
            orig_qty = order_response["origQty"].get<double>();
          }

          if (order_response["orderId"].is_string()) {
            orderId = order_response["orderId"].get<std::string>();
          } else if (order_response["orderId"].is_number()) {
            orderId = std::to_string(order_response["orderId"].get<long>());
          }

          std::cout << "Order after creation: " << orderId << std::endl;
          last_order_id = orderId;
          last_orig_qty = orig_qty;
          last_tp_price = tpPrice;
          last_sl_price = slPrice;
          monitor_lock = false;

          auto response = OrderService::getOrderDetails(apiParams, "BTCUSDT", orderId);
          std::cout << "------------------\nOrders Details Response:\n" << response.dump(4) << std::endl << std::endl;
        }
      });
}

std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> fetchDeactivateDateRange() {
  std::string output = Utils::exec("../run_gsutil_deactivate.sh");
  std::istringstream iss(output);
  std::string line;
  std::unordered_map<std::string, size_t> headerIndex;
  std::string start_str, end_str;

  // Read headers
  if (std::getline(iss, line)) {
    std::istringstream headerStream(line);
    std::string header;
    size_t index = 0;
    while (std::getline(headerStream, header, ',')) {
      headerIndex[header] = index++;
    }
  }

  // Read the data line
  if (std::getline(iss, line)) {
    std::istringstream lineStream(line);
    std::vector<std::string> columns;
    std::string column;
    while (std::getline(lineStream, column, ',')) {
      columns.push_back(column);
    }

    if (!columns.empty()) {
      if (headerIndex.find("start") != headerIndex.end()) {
        start_str = columns[headerIndex["start"]];
      }
      if (headerIndex.find("end") != headerIndex.end()) {
        end_str = columns[headerIndex["end"]];
      }
    }
  }

  auto start_time = parseDateTime(start_str);
  auto end_time = parseDateTime(end_str);
  return {start_time, end_time};
}

namespace Signaling {
std::tuple<std::string, int, double> readSignal() {
  std::string output = Utils::exec("../run_gsutil.sh");
  std::istringstream iss(output);
  std::string line;
  std::unordered_map<std::string, size_t> headerIndex;
  std::string datetime, signal_str, lag_str;
  int signal = 0;
  double lag = 0.0;

  // Read headers
  if (std::getline(iss, line)) {
    std::istringstream headerStream(line);
    std::string header;
    size_t index = 0;
    while (std::getline(headerStream, header, ',')) {
      headerIndex[header] = index++;
    }
  }

  // Read the last line
  std::string lastLine;
  while (std::getline(iss, line)) {
    lastLine = line;
  }

  // Process the last line
  std::istringstream lineStream(lastLine);
  std::vector<std::string> columns;
  std::string column;
  while (std::getline(lineStream, column, ',')) {
    columns.push_back(column);
  }

  if (!columns.empty()) {
    if (headerIndex.find("datetime") != headerIndex.end()) {
      datetime = columns[headerIndex["datetime"]];
    }
    if (headerIndex.find("signal") != headerIndex.end()) {
      signal_str = columns[headerIndex["signal"]];
    }
    if (headerIndex.find("lag") != headerIndex.end()) {
      lag_str = columns[headerIndex["lag"]];
    }

    try {
      signal = std::stoi(signal_str);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Invalid signal value: " << signal_str << std::endl;
    }

    try {
      lag = std::stod(lag_str);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Invalid lag value: " << lag_str << std::endl;
    }
  }

  return {datetime, signal, lag};
}

[[noreturn]] void init(const APIParams &apiParams) {
  SignalQueue signalQueue;
  SignalQueue tpSlQueue;
  std::string prev_datetime;

  std::string last_order_id = "none";
  double last_orig_qty = 0;
  double last_tp_price = 0;
  double last_sl_price = 0;
  bool monitor_lock = true;

  auto newsDateRange = fetchNewsDateRange();

  while (true) {
    if (isCurrentTimeInRange(newsDateRange)) {
      continue;
    }
    // std::cout << "Current datetime is not within the news date range." << std::endl;

    auto deactivateDateRange = fetchDeactivateDateRange();
    std::time_t deactivateMinTime = std::chrono::system_clock::to_time_t(deactivateDateRange.first);
    std::time_t deactivateMaxTime = std::chrono::system_clock::to_time_t(deactivateDateRange.second);
    // std::cout << "Deactivate Date Range: " << std::put_time(std::localtime(&deactivateMinTime), "%Y-%m-%d %H:%M:%S")
    // << " to " << std::put_time(std::localtime(&deactivateMaxTime), "%Y-%m-%d %H:%M:%S") << std::endl;

    if (isCurrentTimeInRange(deactivateDateRange)) {
      continue;
    }
    // std::cout << "Current datetime is NOT within the deactivate date range." << std::endl;

    auto [datetime, signal, lag] = readSignal();

    if (!monitor_lock) {
      std::cout << "-> Adding new monitor event. \n";
      monitorOrderAndPlaceTpSl(tpSlQueue, apiParams, "BTCUSDT", signal == 1 ? "SELL" : "BUY", last_order_id,
                               last_orig_qty, last_tp_price, last_sl_price, monitor_lock);
    }

    if (signal == 0) {
      // std::cout << "Signaling received: DO NOTHING" << std::endl;
      continue;
    }

    if (datetime.empty()) {
      // std::cout << "No valid signal received." << std::endl;
      continue;
    }

    if (datetime == prev_datetime) {
      // std::cout << "Signal datetime has not changed. Skipping execution." << std::endl;
      continue;
    }

    prev_datetime = datetime;

    if (signal == 1) {
      processSignal(signal, apiParams, signalQueue, "BUY", last_order_id, last_orig_qty, last_tp_price, last_sl_price,
                    monitor_lock);
      cancelWithDelay(signal, apiParams, signalQueue, tpSlQueue, monitor_lock);
    } else if (signal == -1) {
      processSignal(signal, apiParams, signalQueue, "SELL", last_order_id, last_orig_qty, last_tp_price, last_sl_price,
                    monitor_lock);
      cancelWithDelay(signal, apiParams, signalQueue, tpSlQueue, monitor_lock);
    }
  }
}
} // namespace Signaling
