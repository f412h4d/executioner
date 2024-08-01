#include <thread>
#include <vector>
#include <memory>
#include <cpr/cpr.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "utils.h"
#include "APIParams.h"
#include "modules/Websocket/headers/root_certificates.hpp"
#include "modules/Websocket/headers/session.hpp"
#include "modules/Websocket/headers/price_session.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

// Function to run a WebSocket session
void run_websocket_session(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<ssl::context> ctx, const std::string& host, const std::string& port) {
    std::make_shared<session>(*ioc, *ctx)->run(host, port);
    ioc->run();
}

void run_custom_websocket_session(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<ssl::context> ctx, const std::string& host, const std::string& port) {
    std::make_shared<price_session>(*ioc, *ctx)->run(host, port);
    ioc->run();
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    auto logger = spdlog::stdout_color_mt("my_logger");
    logger->info("This is an info message");
    logger->warn("This is a warning message");
    logger->error("This is an Error message");

    std::string exePath = Utils::getExecutablePath();
    std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
    std::string envFilePath = exeDir + "/../.env";
    std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

    bool useTestnet = (env["TESTNET"] == "TRUE");
    std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
    std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

    APIParams apiParams(
            apiKey,
            apiSecret,
            5000,
            env["TESTNET"] == "TRUE"
    );

    // List of threads to run WebSocket sessions
    std::vector<std::thread> threads;

    auto ioc = std::make_shared<boost::asio::io_context>();
    auto ctx = std::make_shared<ssl::context>(ssl::context::sslv23_client);
    load_root_certificates(*ctx);

    // Run the WebSocket session in a new thread
    threads.emplace_back(run_websocket_session, ioc, ctx, "fstream.binance.com", "443");
    threads.emplace_back(run_custom_websocket_session, ioc, ctx, "fstream.binance.com", "443");

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return EXIT_SUCCESS;
}
