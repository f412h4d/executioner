#include "signaling.h"
#include "order.h"
#include "margin.h"
#include "utils.h"
#include "../../TimedEventQueue/headers/SignalQueue.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>

#define EXEC_DELAY 15
#define CANCEL_DELAY 315
#define CALC_PRICE_PERCENTAGE (-0.01)
#define TP_PRICE_PERCENTAGE 0.014
#define SL_PRICE_PERCENTAGE (-0.012)

#define TICK_SIZE 0.1

bool prepareForOrder(const APIParams &apiParams) {
    std::string notional;
    size_t array_length;

    auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
    if (positions_response.is_array() && positions_response[0].contains("notional")) {
        notional = positions_response[0]["notional"];
    } else {
        std::cerr << "Notional not found in the response" << std::endl;
        return false;
    }

    auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
    if (open_orders_response.is_array()) {
        array_length = open_orders_response.size();
    } else {
        std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
        return false;
    }

    if (notional != "0") {
        std::cerr << "Notional is not 0. skipping to the next signal" << std::endl;
        return false;
    }

    if (array_length == 3) {
        std::cerr << "There are 3 opening orders. skipping to the next signal" << std::endl;
        return false;
    }

    if (array_length == 1) {
        std::cout << "There is 1 opening orders, cleaning up!!" << std::endl;
        auto response = OrderService::cancelAllOpenOrders(apiParams, "BTCUSDT");
        std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;

        return true;
    }

    return true;
}

double roundToTickSize(double price, double tick_size) {
    return std::round(price / tick_size) * tick_size;
}

namespace Signaling {
    std::pair<std::string, int> fetchSignal() {
        std::string output = Utils::exec("../run_gsutil.sh");
        std::istringstream iss(output);
        std::string datetime, signal_str;
        int signal = 0;

        if (std::getline(iss, datetime, ',') && std::getline(iss, signal_str)) {
            try {
                signal = std::stoi(signal_str);
            } catch (const std::invalid_argument &e) {
                std::cerr << "Invalid signal value: " << signal_str << std::endl;
            }
        }

        return {datetime, signal};
    }

    [[noreturn]] void mockSignal(const APIParams &apiParams) {
        SignalQueue signalQueue;
        std::string prev_datetime;

        while (true) {
            auto [datetime, signal] = fetchSignal();

            std::cout << "Signal -> " << signal << std::endl;
            std::cout << "Datetime -> " << datetime << std::endl;

            if (signal == 0) {
                std::cout << "Signaling received: DO NOTHING" << std::endl;
                continue;
            }

            if (datetime.empty()) {
                std::cout << "No valid signal received." << std::endl;
                continue;
            }

            if (datetime == prev_datetime) {
                std::cout << "Signal datetime has not changed. Skipping execution." << std::endl;
                continue;
            }

            prev_datetime = datetime;

            if (signal == 1) {
                std::cout << "Signaling received: BUY" << std::endl;

                std::cout << "Signal " << signal << " is going to be executed in " +
                                                    std::to_string(EXEC_DELAY) +
                                                    " seconds" << std::endl;

                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(EXEC_DELAY),
                        "Signal is executed.",
                        [&apiParams, signal]() {
                            bool validConditions = prepareForOrder(apiParams);
                            if (!validConditions) {
                                return;
                            }

                            auto price = Margin::getPrice(apiParams, "BTCUSDT");
                            double calculated_price = roundToTickSize(price * (1 + (CALC_PRICE_PERCENTAGE * signal)), TICK_SIZE);
                            double tpPrice = roundToTickSize(price * (1 + (TP_PRICE_PERCENTAGE * signal)), TICK_SIZE);
                            double slPrice = roundToTickSize(price * (1 + (SL_PRICE_PERCENTAGE * signal)), TICK_SIZE);

                            OrderInput order(
                                    "BTCUSDT",
                                    "BUY",
                                    "LIMIT",
                                    "GTC",
                                    0.005,
                                    calculated_price
                            );

                            auto order_response = OrderService::createOrder(apiParams, order);
                            std::cout << "Order Response: " << order_response.dump(4) << std::endl;

                            if (order_response.contains("orderId")) {
                                // Extract the original quantity from the order response
                                double orig_qty = std::stod(order_response["origQty"].get<std::string>());

                                // Take Profit Order
                                TriggerOrderInput tpOrder(
                                        "BTCUSDT",
                                        "SELL",
                                        "TAKE_PROFIT",
                                        "GTC",
                                        orig_qty,
                                        tpPrice,
                                        tpPrice
                                );
                                auto tp_response = OrderService::createTriggerOrder(apiParams, tpOrder);
                                std::cout << "TP Order Response: " << tp_response.dump(4) << std::endl;

                                // Stop Loss Order
                                TriggerOrderInput slOrder(
                                        "BTCUSDT",
                                        "SELL",
                                        "STOP",
                                        "GTC",
                                        orig_qty,
                                        slPrice,
                                        slPrice
                                );
                                auto sl_response = OrderService::createTriggerOrder(apiParams, slOrder);
                                std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
                            }
                        }
                );

                std::cout << "Signal #" + std::to_string(signal) + " Added to queue to be canceled" << std::endl;
                // TODO maybe define cancel queue?
                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(CANCEL_DELAY),
                        "Trying to cancel the order " + std::to_string(signal),
                        [&apiParams]() {
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
                            } else {
                                std::cerr << "Unexpected response format: " << open_orders_response.dump(4)
                                          << std::endl;
                            }
                        }
                );
            } else if (signal == -1) {
                std::cout << "Signaling received: SELL" << std::endl;

                std::cout << "Signal #" + std::to_string(signal) +
                             " is going to be canceled in " +
                             std::to_string(CANCEL_DELAY) + " seconds." << std::endl;

                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(EXEC_DELAY),
                        "Signal #" + std::to_string(signal) + " is executed.",
                        [&apiParams, signal]() {
                            bool validConditions = prepareForOrder(apiParams);
                            if (!validConditions) {
                                return;
                            }

                            auto price = Margin::getPrice(apiParams, "BTCUSDT");
                            double calculated_price = roundToTickSize(price * (1 + (CALC_PRICE_PERCENTAGE * signal)), TICK_SIZE);
                            double tpPrice = roundToTickSize(price * (1 + (TP_PRICE_PERCENTAGE * signal)), TICK_SIZE);
                            double slPrice = roundToTickSize(price * (1 + (SL_PRICE_PERCENTAGE * signal)), TICK_SIZE);

                            OrderInput order(
                                    "BTCUSDT",
                                    "SELL",
                                    "LIMIT",
                                    "GTC",
                                    0.005,
                                    calculated_price
                            );

                            auto order_response = OrderService::createOrder(apiParams, order);
                            std::cout << "Order Response: " << order_response.dump(4) << std::endl;

                            if (order_response.contains("orderId")) {
                                // Extract the original quantity from the order response
                                double orig_qty = std::stod(order_response["origQty"].get<std::string>());

                                // Take Profit Order
                                TriggerOrderInput tpOrder(
                                        "BTCUSDT",
                                        "BUY",
                                        "TAKE_PROFIT",
                                        "GTC",
                                        orig_qty,
                                        tpPrice,
                                        tpPrice
                                );
                                auto tp_response = OrderService::createTriggerOrder(apiParams, tpOrder);
                                std::cout << "TP Order Response: " << tp_response.dump(4) << std::endl;

                                // Stop Loss Order
                                TriggerOrderInput slOrder(
                                        "BTCUSDT",
                                        "BUY",
                                        "STOP",
                                        "GTC",
                                        orig_qty,
                                        slPrice,
                                        slPrice
                                );
                                auto sl_response = OrderService::createTriggerOrder(apiParams, slOrder);
                                std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
                            }
                        }
                );

                std::cout << "Signal #" + std::to_string(signal) +
                             " is going to be canceled in " +
                             std::to_string(CANCEL_DELAY) + " seconds.";

                // TODO maybe define cancel queue?
                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(CANCEL_DELAY),
                        "Signal #" + std::to_string(signal) + " is executed.",
                        [&apiParams]() {

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

                            auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
                            if (open_orders_response.is_array() && !open_orders_response.empty()) {
                                auto response = OrderService::cancelAllOpenOrders(apiParams, "BTCUSDT");
                                std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;
                            } else {
                                std::cerr << "Unexpected response format: " << open_orders_response.dump(4)
                                          << std::endl;
                            }
                        }
                );
            }
        }
    }
}
