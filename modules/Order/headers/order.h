#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include "../models/APIParams/APIParams.h"
#include "../models/OrderInput/OrderInput.h"
#include "../models/TriggerOrderInput/TriggerOrderInput.h"
#include "nlohmann/json.hpp"

class OrderService {
public:
    static nlohmann::json createOrder(const APIParams &apiParams, const OrderInput &order);
    static nlohmann::json createTriggerOrder(const APIParams &apiParams, const TriggerOrderInput &triggerOrder);
    static nlohmann::json cancelAllOpenOrders(const APIParams &apiParams, const std::string &symbol);
    static nlohmann::json getOrderDetails(const APIParams &apiParams, const std::string &symbol, const std::string &orderId = "", const std::string &origClientOrderId = "");
};

#endif // ORDER_SERVICE_H
