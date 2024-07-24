#include "../headers/order.h"
#include "../../Utils/headers/utils.h"
#include "cpr/cpr.h"
#include <iostream>
#include <ctime>

nlohmann::json OrderService::createOrder(const APIParams &apiParams, const OrderInput &order) {
    std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
    std::string apiCall = "fapi/v1/order";

    long timestamp = static_cast<long>(std::time(nullptr) * 1000);

    std::string params =
            "symbol=" + order.symbol + "&side=" + order.side + "&type=" + order.type + "&timeInForce=" +
            order.timeInForce +
            "&quantity=" + std::to_string(order.quantity) + "&recvWindow=" + std::to_string(apiParams.recvWindow) +
            "&timestamp=" + std::to_string(timestamp);

    if (order.type != "MARKET") {
        params += "&price=" + std::to_string(order.price);
    }

    std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
    std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

    cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
    std::cout << "Response Code: " << r.status_code << std::endl;
    std::cout << "Response Text: " << r.text << std::endl;

    return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::createTriggerOrder(const APIParams &apiParams, const TriggerOrderInput &triggerOrder) {
    std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
    std::string apiCall = "fapi/v1/order";

    long timestamp = static_cast<long>(std::time(nullptr) * 1000);

    std::string params =
            "symbol=" + triggerOrder.symbol + "&side=" + triggerOrder.side + "&type=" + triggerOrder.type +
            "&quantity=" + std::to_string(triggerOrder.quantity) + "&recvWindow=" +
            std::to_string(apiParams.recvWindow) +
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
    std::cout << "Response Code: " << r.status_code << std::endl;
    std::cout << "Response Text: " << r.text << std::endl;

    return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::cancelAllOpenOrders(const APIParams &apiParams, const std::string &symbol) {
    std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
    std::string apiCall = "fapi/v1/allOpenOrders";

    long timestamp = static_cast<long>(std::time(nullptr) * 1000);

    std::string params =
            "symbol=" + symbol + "&recvWindow=" + std::to_string(apiParams.recvWindow) +
            "&timestamp=" + std::to_string(timestamp);

    std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
    std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

    cpr::Response r = cpr::Delete(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
    std::cout << "Response Code: " << r.status_code << std::endl;
    std::cout << "Response Text: " << r.text << std::endl;

    return nlohmann::json::parse(r.text);
}

nlohmann::json OrderService::getOrderDetails(const APIParams &apiParams, const std::string &symbol, const std::string &orderId, const std::string &origClientOrderId) {
    std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
    std::string apiCall = "fapi/v1/order";

    long timestamp = static_cast<long>(std::time(nullptr) * 1000);

    std::string params = "symbol=" + symbol + "&recvWindow=" + std::to_string(apiParams.recvWindow) + "&timestamp=" + std::to_string(timestamp);

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
    std::cout << "Response Code: " << r.status_code << std::endl;
    std::cout << "Response Text: " << r.text << std::endl;

    return nlohmann::json::parse(r.text);
}
