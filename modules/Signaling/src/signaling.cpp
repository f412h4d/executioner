#include "../headers/signaling.h"
#include "../../Order/headers/order.h"
#include "../../Margin/headers/margin.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>

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
        std::vector<int> signals = readSignalsFromFile(filePath);

        OrderService orderService;

        for (size_t i = 0; i < signals.size(); ++i) {
            int signal = signals[i];
            bool isLastSignal = (i == signals.size() - 1);

            if (signal == 0) {
                std::cout << "Signaling received: DO NOTHING" << std::endl;
                continue;
            }

            std::string notional;
            size_t array_length;

            double tpPrice;
            double slPrice;
            double calculated_price;

            if (signal == 1) {
                calculated_price = 50000;
                tpPrice = 71000.0; // Example TP price
                slPrice = 45000.0; // Example SL price
            } else if (signal == -1) {
                calculated_price = 80000;
                slPrice = 71000.0; // Example TP price
                tpPrice = 45000.0; // Example SL price
            }

            auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
            std::cout << "Index: " << i << "\t Positions: " << positions_response.dump(4) << std::endl;

            if (positions_response.is_array() && positions_response[0].contains("notional")) {
                notional = positions_response[0]["notional"];
            } else {
                std::cerr << "Notional not found in the response" << std::endl;
                break;
            }

            auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
            std::cout << "Index: " << i << "\t Orders: " << open_orders_response.dump(4) << std::endl;

            if (open_orders_response.is_array()) {
                array_length = open_orders_response.size();
            } else {
                std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
                break;
            }

            if (array_length != 0 || notional != "0") {
                std::cerr << "Either Notional or Open Orders are not 0. skipping to the next signal" << std::endl;
                continue;
            }

            if (signal == 1) {
                std::cout << "Signaling received: BUY" << std::endl;
                OrderInput order(
                        "BTCUSDT",
                        "BUY",
                        "LIMIT",
                        "GTC",
                        0.01,
                        calculated_price
                );
                auto order_response = orderService.createOrder(apiParams, order);
                std::cout << "Order Response: " << order_response.dump(4) << std::endl;

                if (order_response.contains("orderId")) {
//                    open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
//                    if (open_orders_response.is_array()) {
//                        array_length = open_orders_response.size();
//                        auto response = orderService.cancelAllOpenOrders(apiParams, "BTCUSDT");
//                        std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;
//                    } else {
//                        std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
//                    }

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
                    auto tp_response = orderService.createTriggerOrder(apiParams, tpOrder);
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
                    auto sl_response = orderService.createTriggerOrder(apiParams, slOrder);
                    std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
                }
            } else if (signal == -1) {
                std::cout << "Signaling received: SELL" << std::endl;
                OrderInput order(
                        "BTCUSDT",
                        "SELL",
                        "LIMIT",
                        "GTC",
                        0.01,
                        calculated_price
                );
                auto order_response = orderService.createOrder(apiParams, order);
                std::cout << "Order Response: " << order_response.dump(4) << std::endl;

                if (order_response.contains("orderId")) {
//                    open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
//                    if (open_orders_response.is_array()) {
//                        array_length = open_orders_response.size();
//                        auto response = orderService.cancelAllOpenOrders(apiParams, "BTCUSDT");
//                        std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;
//                    } else {
//                        std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
//                    }

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
                    auto tp_response = orderService.createTriggerOrder(apiParams, tpOrder);
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
                    auto sl_response = orderService.createTriggerOrder(apiParams, slOrder);
                    std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
                }
            }

            // Mimic a delay of 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Check if this is the last signal
            if (isLastSignal) {
                std::cout << "This was the last signal." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(180));

                // Add any additional logic to handle the last signal here
            }
        }
    }
}
