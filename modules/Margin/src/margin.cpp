#include "../headers/margin.h"
#include "../../Utils/headers/utils.h"
#include "cpr/cpr.h"
#include <iostream>
#include <ctime>

namespace Margin {
    std::string getBaseUrl(const APIParams &apiParams) {
        return apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
    }

    std::string generateSignature(const std::string &secret, const std::string &query_string) {
        return Utils::HMAC_SHA256(secret, query_string);
    }

    std::string urlEncode(const std::string &value) {
        return cpr::util::urlEncode(value);
    }

    double getPrice(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = getBaseUrl(apiParams);
        std::string apiCall = "fapi/v1/ticker/price";
        std::string url = baseUrl + "/" + apiCall + "?symbol=" + symbol;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        return std::stod(nlohmann::json::parse(r.text)["price"].get<std::string>());
    }

    nlohmann::json getPositions(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = getBaseUrl(apiParams);
        std::string apiCall = "fapi/v2/positionRisk";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp) + "&recvWindow=" + std::to_string(apiParams.recvWindow);
        if (!symbol.empty()) {
            params += "&symbol=" + symbol;
        }

        std::string signature = generateSignature(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getOpenOrders(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = getBaseUrl(apiParams);
        std::string apiCall = "fapi/v1/openOrders";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp) + "&recvWindow=" + std::to_string(apiParams.recvWindow);
        if (!symbol.empty()) {
            params += "&symbol=" + symbol;
        }

        std::string signature = generateSignature(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getBalance(
            const APIParams &apiParams,
            const std::string &asset
    ) {
        std::string baseUrl = getBaseUrl(apiParams);
        std::string apiCall = "fapi/v2/account";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp) + "&recvWindow=" + std::to_string(apiParams.recvWindow);

        std::string signature = generateSignature(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + urlEncode(signature);

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        nlohmann::json jsonResponse = nlohmann::json::parse(r.text);

        for (const auto &balance: jsonResponse["assets"]) {
            if (balance["asset"] == asset) {
                return balance;
            }
        }

        return nlohmann::json::object();
    }
}
