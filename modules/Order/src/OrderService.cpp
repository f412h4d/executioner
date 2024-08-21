#include <ctime>
#include <string>
#include <tuple>

#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "../../Utils/headers/utils.h"
#include "../headers/order.h"

nlohmann::json OrderService::createOrder(const APIParams &apiParams, const OrderInput &order) {
  auto order_logger = spdlog::get("order_logger");
  std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
  std::string apiCall = "fapi/v1/order";

  long timestamp = static_cast<long>(std::time(nullptr) * 1000);

  std::string params = "symbol=" + order.symbol + "&side=" + order.side + "&type=" + order.type +
                       "&timeInForce=" + order.timeInForce + "&quantity=" + std::to_string(order.quantity) +
                       "&recvWindow=" + std::to_string(apiParams.recvWindow) +
                       "&timestamp=" + std::to_string(timestamp);

  if (order.type != "MARKET") {
    params += "&price=" + std::to_string(order.price);
  }

  std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
  std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

  cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

  order_logger->debug("Create Order Response Code: {}", r.status_code);
  order_logger->debug("Create Order Response Text: {}", r.text);

  return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::createTriggerOrder(const APIParams &apiParams, const TriggerOrderInput &triggerOrder) {
  auto order_logger = spdlog::get("order_logger");
  std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
  std::string apiCall = "fapi/v1/order";

  long timestamp = static_cast<long>(std::time(nullptr) * 1000);

  std::string params = "symbol=" + triggerOrder.symbol + "&side=" + triggerOrder.side + "&type=" + triggerOrder.type +
                       "&quantity=" + std::to_string(triggerOrder.quantity) +
                       "&recvWindow=" + std::to_string(apiParams.recvWindow) +
                       "&timestamp=" + std::to_string(timestamp);

  if (triggerOrder.type == "STOP_MARKET" || triggerOrder.type == "TAKE_PROFIT_MARKET") {
    params += "&stopPrice=" + std::to_string(triggerOrder.stopPrice);
  } else {
    params += "&price=" + std::to_string(triggerOrder.price) + "&stopPrice=" + std::to_string(triggerOrder.stopPrice);
  }

  if (triggerOrder.reduceOnly) {
    params += "&reduceOnly=true";
  }

  std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
  std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

  cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

  order_logger->debug("Create Trigger Order Response Code: {}", r.status_code);
  order_logger->debug("Create Trigger Order Response Text: {}", r.text);

  return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::cancelSingleOrder(const APIParams &apiParams, const std::string &symbol,
                                               const std::string &orderId, const std::string &origClientOrderId) {
  auto order_logger = spdlog::get("order_logger");
  std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
  std::string apiCall = "fapi/v1/order";

  long timestamp = static_cast<long>(std::time(nullptr) * 1000);

  std::string params = "symbol=" + symbol + "&recvWindow=" + std::to_string(apiParams.recvWindow) +
                       "&timestamp=" + std::to_string(timestamp);

  if (!orderId.empty()) {
    params += "&orderId=" + orderId;
  } else if (!origClientOrderId.empty()) {
    params += "&origClientOrderId=" + origClientOrderId;
  } else {
    order_logger->error("Either orderId or origClientOrderId must be provided.");
    throw std::invalid_argument("Either orderId or origClientOrderId must be provided.");
  }

  std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
  std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

  cpr::Response r = cpr::Delete(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

  order_logger->debug("Cancel Single Order Response Code: {}", r.status_code);
  order_logger->debug("Cancel Single Order Response Text: {}", r.text);

  return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::cancelAllOpenOrders(const APIParams &apiParams, const std::string &symbol) {
  auto order_logger = spdlog::get("order_logger");
  std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
  std::string apiCall = "fapi/v1/allOpenOrders";

  long timestamp = static_cast<long>(std::time(nullptr) * 1000);

  std::string params = "symbol=" + symbol + "&recvWindow=" + std::to_string(apiParams.recvWindow) +
                       "&timestamp=" + std::to_string(timestamp);

  std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
  std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

  cpr::Response r = cpr::Delete(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

  order_logger->debug("Cancel Order Response Code: {}", r.status_code);
  order_logger->debug("Cancel Order Response Text: {}", r.text);

  return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::getOrderDetails(const APIParams &apiParams, const std::string &symbol,
                                             const std::string &orderId, const std::string &origClientOrderId) {
  auto order_logger = spdlog::get("order_logger");
  std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
  std::string apiCall = "fapi/v1/order";

  long timestamp = static_cast<long>(std::time(nullptr) * 1000);

  std::string params = "symbol=" + symbol + "&recvWindow=" + std::to_string(apiParams.recvWindow) +
                       "&timestamp=" + std::to_string(timestamp);

  if (!orderId.empty()) {
    params += "&orderId=" + orderId;
  } else if (!origClientOrderId.empty()) {
    params += "&origClientOrderId=" + origClientOrderId;
  } else {
    throw std::invalid_argument("Either orderId or origClientOrderId must be provided.");
  }

  std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
  std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

  cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

  order_logger->debug("Order Detials Response Code: {}", r.status_code);
  order_logger->debug("Order Details Response Text: {}", r.text);

  return nlohmann::json::parse(r.text);
}

std::tuple<std::string, std::string, double, double> validateOrderResponse(const nlohmann::json &order_response) {
  auto exec_logger = spdlog::get("exec_logger");

  if (!order_response.contains("orderId") || !order_response["orderId"].is_number()) {
    exec_logger->critical("Failure in createOrder, orderId not found or invalid in the response. response: {}",
                          order_response);
    throw std::runtime_error("Invalid or missing orderId in order response.");
  }
  std::string order_id = std::to_string(order_response["orderId"].get<long>());

  if (!order_response.contains("side") || !order_response["side"].is_string()) {
    exec_logger->critical("Failure in createOrder, side not found or invalid in the response. response: {}",
                          order_response);
    throw std::runtime_error("Invalid or missing side in order response.");
  }
  std::string side = order_response["side"].get<std::string>();

  if (!order_response.contains("origQty") || !order_response["origQty"].is_string()) {
    exec_logger->critical("Failure in createOrder, origQty not found or invalid in the response. response: {}",
                          order_response);
    throw std::runtime_error("Invalid or missing origQty in order response.");
  }
  double quantity = std::stod(order_response["origQty"].get<std::string>());

  if (!order_response.contains("price") || !order_response["price"].is_string()) {
    exec_logger->critical("Failure in createOrder, price not found or invalid in the response. response: {}",
                          order_response);
    throw std::runtime_error("Invalid or missing price in order response.");
  }
  double price = std::stod(order_response["price"].get<std::string>());

  return std::make_tuple(order_id, side, quantity, price);
}
