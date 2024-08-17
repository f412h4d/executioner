#ifndef TRADE_SIGNAL_H
#define TRADE_SIGNAL_H

#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

struct TradeSignal {
  int lag;              // Lag in seconds
  int value;            // 0, 1, or -1
  std::string datetime; // Date and time in UTC format "2024-08-05 14:00:00"

  std::chrono::system_clock::time_point toTimePoint() const {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
  }

  std::string toJsonString() const {
    nlohmann::json json_message = {{"lag", lag}, {"value", value}, {"datetime", datetime}};
    return json_message.dump();
  }

  static TradeSignal fromJsonString(const std::string &json_string) {
    TradeSignal signal;
    nlohmann::json json_message = nlohmann::json::parse(json_string);

    signal.lag = json_message["lag"].get<int>();
    signal.value = json_message["value"].get<int>();
    signal.datetime = json_message["datetime"].get<std::string>();

    return signal;
  }
};

#endif // TRADE_SIGNAL_H
