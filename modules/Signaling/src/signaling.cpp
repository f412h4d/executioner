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

#define EXEC_DELAY 2 // Entry Time offset
#define CANCEL_DELAY 35 // Open Order Elimination
#define MONITOR_DELAY 1
#define CALC_PRICE_PERCENTAGE (0) // Entry Gap
#define TP_PRICE_PERCENTAGE 0.01
#define SL_PRICE_PERCENTAGE (-0.01)

#define TICK_SIZE 0.1

bool prepareForOrder(const APIParams &apiParams) {
    std::string notional;
    size_t array_length;

    auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
    if (positions_response.is_array() && positions_response[0].contains("notional")) {
        notional = positions_response[0]["notional"].get<std::string>();
    } else {
        std::cerr << "Notional not found in the response" << std::endl;
        return false;
    }

    auto open_orders_response = Margin::getOpenOrders(apiParams, "BTCUSDT");
    std::cout << "\n\nLogs:\n\n";
    std::cout << "Positions Resp:\n" << positions_response.dump(4) << "\n------------------------";
    std::cout << "Open Orders Resp:\n" << open_orders_response.dump(4) << "\n\n\n\n";

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

    if (array_length >= 1) {
        for (const auto& order : open_orders_response) {
            if (order.contains("origType") && order["origType"] == "LIMIT") {
                return false;
            }
        }

        auto response = OrderService::cancelAllOpenOrders(apiParams, "BTCUSDT");
        std::cout << "Cancel All Orders Response: " << response.dump(4) << std::endl;
    }

    return true;
}

double roundToTickSize(double price, double tick_size) {
    return std::round(price / tick_size) * tick_size;
}

void placeTpAndSlOrders(const APIParams &apiParams, const std::string &symbol, const std::string &side, double orig_qty, double tpPrice,
                        double slPrice) {
    std::cout << "Params:\n" << orig_qty << "\t" << tpPrice << "\t" << slPrice;

    TriggerOrderInput tpOrder(
            symbol,
            side,
            "TAKE_PROFIT_MARKET",
            "GTC",
            orig_qty,
            tpPrice,
            tpPrice,
            true
    );
    auto tp_response = OrderService::createTriggerOrder(apiParams, tpOrder);
    std::cout << "TP Order Response: " << tp_response.dump(4) << std::endl;

    TriggerOrderInput slOrder(
            symbol,
            side,
            "STOP_MARKET",
            "GTC",
            orig_qty,
            slPrice,
            slPrice,
            true
    );
    auto sl_response = OrderService::createTriggerOrder(apiParams, slOrder);
    std::cout << "SL Order Response: " << sl_response.dump(4) << std::endl;
}

bool isOrderFilled(const APIParams &apiParams) {
    std::string notional;

    auto positions_response = Margin::getPositions(apiParams, "BTCUSDT");
    if (positions_response.is_array() && positions_response[0].contains("notional")) {
        notional = positions_response[0]["notional"].get<std::string>();
    } else {
        std::cerr << "Notional not found in the response" << std::endl;
        return false;
    }

    return notional != "0";
}

void monitorOrderAndPlaceTpSl(SignalQueue &signalQueue,
                              const APIParams &apiParams,
                              const std::string &symbol,
                              const std::string &side,
                              double &orig_qty,
                              double &tpPrice,
                              double &slPrice,
                              bool &monitor_lock) {
    std::cout << "Monitor Order Status will run in 1 secs.\n";
    signalQueue.addEvent(
            TIME::now() + std::chrono::seconds(MONITOR_DELAY),
            "Monitor Order Status",
            [&apiParams, &symbol, &side, &orig_qty, &tpPrice, &slPrice, &monitor_lock]() {
                std::cout << "\n\n\nTP SL MONITOR Params:\n" << orig_qty << "\t" << tpPrice << "\t" << slPrice << "\n\n\n";

                if (monitor_lock) {
                    std::cout << "Monitoring is locked, waiting for the order to be executed.\n";
                    std::cout << "LOCKED Params:\n" << orig_qty << "\t" << tpPrice << "\t" << slPrice << "\n";
                    return;
                }

                if (isOrderFilled(apiParams)) {
                    std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n" << "Order Is FILLED Adding TP & SL\n";
                    monitor_lock = true;
                    placeTpAndSlOrders(apiParams, symbol, side, orig_qty, tpPrice, slPrice);
                } else {
                    std::cout << "NOT FILLED Params:\n" << orig_qty << "\t" << tpPrice << "\t" << slPrice << "\n";
                    std::cout << "Not filled yet, will check again later.\n";
                }
            }
    );
}

void cancelWithDelay(int signal,
                     const APIParams &apiParams,
                     SignalQueue &signalQueue,
                     SignalQueue &tpSlQueue,
                     bool &lock
                     ) {
    std::cout << "Signal #" + std::to_string(signal) + " Added to queue to be canceled" << std::endl;
    signalQueue.addEvent(
            TIME::now() + std::chrono::seconds(CANCEL_DELAY),
            "Trying to cancel the order " + std::to_string(signal),
            [&apiParams, &tpSlQueue, &lock]() {
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

//                    tpSlQueue.stop();
                    lock = true;
                } else {
                    std::cerr << "Unexpected response format: " << open_orders_response.dump(4) << std::endl;
                }
            }
    );
}

void processSignal(int signal,
                   const APIParams &apiParams,
                   SignalQueue &signalQueue,
                   const std::string &side,
                   double &last_orig_qty,
                   double &last_tp_price,
                   double &last_sl_price,
                   bool &monitor_lock
) {
    std::cout << "Signaling received: " << side << std::endl;
    std::cout << "Signal " << signal << " is going to be executed in " + std::to_string(EXEC_DELAY) + " seconds"
              << std::endl;

    signalQueue.addEvent(
            TIME::now() + std::chrono::seconds(EXEC_DELAY),
            "Signal is executed.",
            [&apiParams, signal, side, &last_orig_qty, &last_tp_price, &last_sl_price, &monitor_lock]() {
                bool validConditions = prepareForOrder(apiParams);
                if (!validConditions) {
                    return;
                }

                auto price = Margin::getPrice(apiParams, "BTCUSDT");
                double orig_price = price * (1 + (CALC_PRICE_PERCENTAGE * signal));
                double calculated_price = roundToTickSize(orig_price, TICK_SIZE);
                std::cout << "Orig:\t" << orig_price << std::endl;
                std::cout << "Rounded:\t" << calculated_price << std::endl;
                double tpPrice = roundToTickSize(calculated_price * (1 + (TP_PRICE_PERCENTAGE * signal)), TICK_SIZE);
                double slPrice = roundToTickSize(calculated_price * (1 + (SL_PRICE_PERCENTAGE * signal)), TICK_SIZE);

                OrderInput order(
                        "BTCUSDT",
                        side,
                        "LIMIT",
                        "GTC",
                        0.005,
                        calculated_price
                );

                auto order_response = OrderService::createOrder(apiParams, order);
                std::cout << "Order Response: " << order_response.dump(4) << std::endl;

                if (order_response.contains("orderId")) {
                    double orig_qty = 0.0;
                    std::string orderId;

                    if (order_response["origQty"].is_string()) {
                        orig_qty = std::stod(order_response["origQty"].get<std::string>());
                    } else if (order_response["origQty"].is_number()) {
                        orig_qty = order_response["origQty"].get<double>();
                    }

                    last_orig_qty = orig_qty;
                    last_tp_price = tpPrice;
                    last_sl_price = slPrice;
                    monitor_lock = false;
                }
            }
    );
}

namespace Signaling {
    std::pair<std::string, int> readSignal() {
        std::string output = Utils::exec("../run_gsutil.sh");
        std::istringstream iss(output);
        std::string line, datetime, signal_str;
        int signal = 0;

        std::string lastLine;
        while (std::getline(iss, line)) {
            lastLine = line;
        }

        std::istringstream lineStream(lastLine);
        std::vector<std::string> columns;
        std::string column;

        while (std::getline(lineStream, column, ',')) {
            columns.push_back(column);
        }

        if (!columns.empty()) {
            datetime = columns.front();
            signal_str = columns.back();

            try {
                signal = std::stoi(signal_str);
            } catch (const std::invalid_argument &e) {
                std::cerr << "Invalid signal value: " << signal_str << std::endl;
            }
        }

        return {datetime, signal};
    }
    [[noreturn]] void init(const APIParams &apiParams) {
        SignalQueue signalQueue;
        SignalQueue tpSlQueue;
        std::string prev_datetime;

        double last_orig_qty = 0;
        double last_tp_price = 0;
        double last_sl_price = 0;
        bool monitor_lock = true;

        while (true) {
            auto [datetime, signal] = readSignal();

            std::cout << "Signal -> " << signal << std::endl;
            std::cout << "Datetime -> " << datetime << std::endl;

            if (!monitor_lock) {
                std::cout << "-> Adding new monitor event. \n";
                monitorOrderAndPlaceTpSl(tpSlQueue, apiParams, "BTCUSDT", signal == 1 ? "SELL":"BUY", last_orig_qty, last_tp_price,
                                         last_sl_price, monitor_lock);
            }

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
                processSignal(signal, apiParams, signalQueue, "BUY", last_orig_qty, last_tp_price,
                              last_sl_price, monitor_lock);
                cancelWithDelay(signal, apiParams, signalQueue, tpSlQueue, monitor_lock);
            } else if (signal == -1) {
                processSignal(signal, apiParams, signalQueue, "SELL", last_orig_qty, last_tp_price,
                              last_sl_price, monitor_lock);
                cancelWithDelay(signal, apiParams, signalQueue, tpSlQueue, monitor_lock);
            }
        }
    }
}
