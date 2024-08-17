#pragma once

#include "./TimedEventQueue.hpp"
#include "spdlog/spdlog.h"

class SignalQueue : public TimedEventQueue {
protected:
  void onTimestampExpire(const TIMESTAMP &timestamp, const std::string &label) override {
    auto cancel_queue_logger = spdlog::get("cancel_queue_logger");
    cancel_queue_logger->info("Timestamp expired: {} with label: {}",
                              std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count(),
                              label);
  }
};
