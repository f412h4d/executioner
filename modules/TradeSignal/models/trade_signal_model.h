#ifndef TRADE_SIGNAL_H
#define TRADE_SIGNAL_H

#include "logger.h"
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
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

  void fromJsonString(const std::string &json_string) {
    nlohmann::json json_message = nlohmann::json::parse(json_string);

    lag = std::stoi(json_message["lag"].get<std::string>());
    value = std::stoi(json_message["value"].get<std::string>());
    datetime = json_message["datetime"].get<std::string>();
  }
};

#endif // TRADE_SIGNAL_H
