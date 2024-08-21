#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include "nlohmann/json.hpp"

#include "../models/APIParams/APIParams.h"
#include "../models/OrderInput/OrderInput.h"
#include "../models/TriggerOrderInput/TriggerOrderInput.h"

class OrderService {
public:
  static nlohmann::json createOrder(const APIParams &apiParams, const OrderInput &order);
  static nlohmann::json createTriggerOrder(const APIParams &apiParams, const TriggerOrderInput &triggerOrder);
  static nlohmann::json cancelOrder(const APIParams &apiParams, const std::string &symbol,
                                          const std::string &orderId, const std::string &origClientOrderId);
  static nlohmann::json cancelAllOpenOrders(const APIParams &apiParams, const std::string &symbol);
  static nlohmann::json getOrderDetails(const APIParams &apiParams, const std::string &symbol,
                                        const std::string &orderId = "", const std::string &origClientOrderId = "");
  static std::tuple<std::string,std::string, std::string, double, double>
  validateOrderResponse(const nlohmann::json &order_response);
};

#endif // ORDER_SERVICE_H
