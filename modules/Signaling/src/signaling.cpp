#include "signaling.h"
#include "order.h"
#include "margin.h"
#include "../../TimedEventQueue/headers/SignalQueue.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>

#define EXEC_DELAY 5
#define CANCEL_DELAY 1000000

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
        std::cout << "Cancel All Orders Response: " << response.dump(4)
                  << std::endl;

        return true;
    }

    return true;
}


namespace Signaling {
    std::vector<int> readSignalsFromFile(const std::string &filePath) {
        std::vector<int> signals;
        std::ifstream file(filePath);

        if (!file.is_open()) {
            std::cerr << "Unable to open file: " << filePath << std::endl;
            return signals;
        }

        char ch;
        while (file >> ch) {
            if (ch == '1' || ch == '0') {
                signals.push_back(ch - '0');
            } else if (ch == '-') {
                char next_ch;
                file >> next_ch;
                if (next_ch == '1') {
                    signals.push_back(-1);
                }
            }
        }

        file.close();
        return signals;
    }

    void mockSignal(const std::string &filePath, const APIParams &apiParams) {
        SignalQueue signalQueue;

        std::vector<int> signals = readSignalsFromFile(filePath);

        for (size_t i = 0; i < signals.size(); ++i) {
            int signal = signals[i];
            bool isLastSignal = (i == signals.size() - 1);

            if (signal == 0 && !isLastSignal) {
                std::cout << "Signaling received: DO NOTHING" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));

                continue;
            }


            if (signal == 1) {
                std::cout << "Signaling received: BUY" << std::endl;

                std::cout << "Signal #" + std::to_string(i) + " is going to be executed in " +
                             std::to_string(EXEC_DELAY) +
                             " seconds" << std::endl;
                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(EXEC_DELAY),
                        "Signal #" + std::to_string(i) + " is executed.",
                        [&apiParams]() {
                            bool validConditions = prepareForOrder(apiParams);
                            if (!validConditions) {
                                return;
                            }

                            double tpPrice;
                            double slPrice;
                            double calculated_price;

                            calculated_price = 56985;
                            tpPrice = calculated_price + 100; // Example TP price
                            slPrice = calculated_price - 100; // Example SL price

                            OrderInput order(
                                    "BTCUSDT",
                                    "BUY",
                                    "LIMIT",
                                    "GTC",
                                    0.01,
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


                std::cout << "Signal #" + std::to_string(i) + " Added to queue to be canceled" << std::endl;
                // TODO maybe define cancel queue?
                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(CANCEL_DELAY),
                        "Trying to cancel the order number " + std::to_string(i),
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
                                std::cout << "Cancel All Orders Response: " << response.dump(4)
                                          << std::endl;
                            } else {
                                std::cerr << "Unexpected response format: "
                                          << open_orders_response.dump(4)
                                          << std::endl;
                            }
                        }
                );
            } else if (signal == -1) {
                std::cout << "Signaling received: SELL" << std::endl;

                std::cout << "Signal #" + std::to_string(i) +
                             " is going to be canceled in " +
                             std::to_string(CANCEL_DELAY) + "seconds." << std::endl;

                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(EXEC_DELAY),
                        "Signal #" + std::to_string(i) + " is executed.",
                        [&apiParams]() {
                            bool validConditions = prepareForOrder(apiParams);
                            if (!validConditions) {
                                return;
                            }

                            double tpPrice;
                            double slPrice;
                            double calculated_price;
                            calculated_price = 80000;
                            slPrice = 71000.0; // Example TP price
                            tpPrice = 45000.0; // Example SL price

                            OrderInput order(
                                    "BTCUSDT",
                                    "BUY",
                                    "LIMIT",
                                    "GTC",
                                    0.01,
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

                std::cout << "Signal #" + std::to_string(i) +
                             " is going to be canceled in " +
                             std::to_string(CANCEL_DELAY) + "seconds.";

                // TODO maybe define cancel queue?
                signalQueue.addEvent(
                        TIME::now() + std::chrono::seconds(CANCEL_DELAY),
                        "Signal #" + std::to_string(i) + " is executed.",
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
                                std::cout << "Cancel All Orders Response: " << response.dump(4)
                                          << std::endl;
                            } else {
                                std::cerr << "Unexpected response format: "
                                          << open_orders_response.dump(4)
                                          << std::endl;
                            }
                        }
                );
            }

            // Mimic a delay of 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));

            // Check if this is the last signal
            if (isLastSignal) {
                std::cout << "This was the last signal." << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(180));
                signalQueue.stop();

                // Add any additional logic to handle the last signal here
            }
        }
    }
}
