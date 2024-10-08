#include "../headers/margin.h"
#include "../../Utils/headers/utils.h"
#include "cpr/cpr.h"
#include <iostream>
#include <ctime>
#include "nlohmann/json.hpp"

namespace Margin {
    double getPrice(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v1/ticker/price";
        std::string url = baseUrl + "/" + apiCall + "?symbol=" + symbol;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        // FIXME: use logs instead!
        // std::cout << "Response Code: " << r.status_code << std::endl;
        // std::cout << "Response Text: " << r.text << std::endl;

        return std::stod(nlohmann::json::parse(r.text)["price"].get<std::string>());
    }

    nlohmann::json getPositions(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v2/positionRisk";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp);
        if (!symbol.empty()) {
            params += "&symbol=" + symbol;
        }

        std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getOpenOrders(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v1/openOrders";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp);
        if (!symbol.empty()) {
            params += "&symbol=" + symbol;
        }
        params += "&recvWindow=" + std::to_string(apiParams.recvWindow);

        std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        // FIXME: use logs instead!
        // std::cout << "Response Code: " << r.status_code << std::endl;
        // std::cout << "Response Text: " << r.text << std::endl;

        return nlohmann::json::parse(r.text);
    }

    double getBalance(
            const APIParams &apiParams,
            const std::string &asset
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v2/account";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp);

        std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        // FIXME: use logs instead!
        // std::cout << "Response Code: " << r.status_code << std::endl;
        // std::cout << "Response Text: " << r.text << std::endl;

        nlohmann::json jsonResponse = nlohmann::json::parse(r.text);

        // Filter the response to get the balance of the specified asset
        for (const auto &balance: jsonResponse["assets"]) {
            if (balance["asset"] == asset) {
                return std::stod(balance["availableBalance"].get<std::string>());
            }
        }

        return 0;
    }

    nlohmann::json setLeverage(
            const APIParams &apiParams,
            const std::string &symbol,
            int leverage
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v1/leverage";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "symbol=" + symbol + "&leverage=" + std::to_string(leverage) + "&timestamp=" + std::to_string(timestamp);

        std::string signature = Utils::HMAC_SHA256(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + Utils::urlEncode(signature);

        cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});

        return nlohmann::json::parse(r.text);
    }
}
