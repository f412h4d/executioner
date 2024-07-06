#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include "../models/APIParams/APIParams.h"
#include "../models/OrderInput/OrderInput.h"
#include "../models/TriggerOrderInput/TriggerOrderInput.h"
#include "nlohmann/json.hpp"

class OrderService {
public:
    nlohmann::json createOrder(const APIParams &apiParams, const OrderInput &order);

    nlohmann::json createTriggerOrder(const APIParams &apiParams, const TriggerOrderInput &triggerOrder);

    nlohmann::json cancelAllOpenOrders(const APIParams &apiParams, const std::string &symbol);
};

#endif // ORDER_SERVICE_H
