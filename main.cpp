#include <thread>
#include <vector>
#include <memory>

#include "utils.h"
#include "APIParams.h"
#include "modules/Websocket/headers/root_certificates.hpp"
#include "modules/Websocket/headers/price_session.hpp"
#include "modules/Websocket/headers/event_order_update_session.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

template<typename SessionType>
void run_websocket_session(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<ssl::context> ctx, const std::string& host, const std::string& port, const APIParams& api_params) {
    auto session_instance = std::make_shared<SessionType>(*ioc, *ctx, api_params);
    session_instance->run(host, port);
    ioc->run();
}

// Shared state for the calculated price
auto calculated_price = std::make_shared<double>(0.0);
std::mutex price_mutex;

int main() {
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
            useTestnet
    );

    // List of threads to run WebSocket sessions
    std::vector<std::thread> threads;

    auto ioc = std::make_shared<boost::asio::io_context>();
    auto ctx = std::make_shared<ssl::context>(ssl::context::sslv23_client);
    load_root_certificates(*ctx);

    // Run the WebSocket sessions in new threads
    threads.emplace_back([&, ioc, ctx]() {
        auto price_session_instance = std::make_shared<price_session>(*ioc, *ctx, apiParams, calculated_price, price_mutex);
        price_session_instance->run("fstream.binance.com", "443");
        ioc->run();
    });

    auto event_order_session = std::make_shared<event_order_update_session>(*ioc, *ctx, apiParams, calculated_price, price_mutex);
    threads.emplace_back([&]() { event_order_session->run("fstream.binance.com", "443"); ioc->run(); });

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return EXIT_SUCCESS;
}
