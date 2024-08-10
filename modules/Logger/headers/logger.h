// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>

namespace Logger {

    // Initialize the loggers with specific names
    static std::shared_ptr<spdlog::logger> margin_logger = spdlog::stdout_color_mt("margin_logger");
    static std::shared_ptr<spdlog::logger> signal_logger = spdlog::stdout_color_mt("signal_logger");
    static std::shared_ptr<spdlog::logger> order_logger = spdlog::stdout_color_mt("order_logger");
    static std::shared_ptr<spdlog::logger> news_logger = spdlog::stdout_color_mt("news_logger");
    static std::shared_ptr<spdlog::logger> active_logger = spdlog::stdout_color_mt("active_logger");

    // Initialize the logger pattern (optional)
    inline void initLoggerPattern() {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    }

} // namespace Logger

#endif // LOGGER_H

