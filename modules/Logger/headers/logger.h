// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace Logger {

inline void setup_loggers() {
  // Define sinks for console (stdout) and file
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/general.log", true);

  // Set log levels
  console_sink->set_level(spdlog::level::info);
  file_sink->set_level(spdlog::level::debug);

  // Combine sinks into one logger for each logger
  auto create_logger = [&](const std::string &name, const std::string &file_path) {
    if (!spdlog::get(name)) {
      auto specific_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, true);
      specific_file_sink->set_level(spdlog::level::debug);
      std::vector<spdlog::sink_ptr> sinks{console_sink, specific_file_sink};
      auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
      logger->set_level(spdlog::level::debug); // Log level for the logger (debug for the file, info for console)
      spdlog::register_logger(logger);
    }
  };

  // Create all required loggers
  create_logger("cancel_queue_logger", "logs/cancel_queue.log");
  create_logger("cancel_logger", "logs/cancel.log");
  create_logger("balance_logger", "logs/balance.log");
  create_logger("market_logger", "logs/market.log");
  create_logger("account_logger", "logs/acount.log");
  create_logger("margin_logger", "logs/margin.log");
  create_logger("exec_logger", "logs/exec.log");
  create_logger("signal_logger", "logs/signal.log");
  create_logger("order_logger", "logs/order.log");
  create_logger("news_logger", "logs/news.log");
  create_logger("active_logger", "logs/active.log");
  create_logger("pubsub_logger", "logs/pubsub.log");

  // Set the logging pattern for all loggers
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] [%s:%#] [%!] %v");
}

} // namespace Logger

#endif // LOGGER_H
