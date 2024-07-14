#include "../headers/margin.h"
#include "../../Utils/headers/utils.h"
#include "cpr/cpr.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "nlohmann/json.hpp"


namespace Margin {
    std::string hmac_sha256(const std::string& key, const std::string& data) {
        unsigned char* digest;
        digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), nullptr, nullptr);

        char mdString[SHA256_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

        return std::string(mdString);
    }

    std::string url_encode(const std::string &value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            }
        }

        return escaped.str();
    }

    double getPrice(
            const APIParams &apiParams,
            const std::string &symbol
    ) {
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
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
        std::string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        std::string apiCall = "fapi/v2/positionRisk";

        long timestamp = static_cast<long>(std::time(nullptr) * 1000);
        std::string params = "timestamp=" + std::to_string(timestamp);
        if (!symbol.empty()) {
            params += "&symbol=" + symbol;
        }

        std::string signature = hmac_sha256(apiParams.apiSecret, params);
        std::string url = baseUrl + "/" + apiCall + "?" + params + "&signature=" + url_encode(signature);

        // Log details
        std::cout << "Base URL: " << baseUrl << std::endl;
        std::cout << "API Key Length: " << apiParams.apiKey.length() << std::endl;
        std::cout << "API Secret Length: " << apiParams.apiSecret.length() << std::endl;
        std::cout << "URL: " << url << std::endl;
        std::cout << "Params: " << params << std::endl;
        std::cout << "Signature: " << signature << std::endl;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

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
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getBalance(
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
        std::cout << "Response Code: " << r.status_code << std::endl;
        std::cout << "Response Text: " << r.text << std::endl;

        nlohmann::json jsonResponse = nlohmann::json::parse(r.text);

        // Filter the response to get the balance of the specified asset
        for (const auto &balance: jsonResponse["assets"]) {
            if (balance["asset"] == asset) {
                return balance;
            }
        }

        // If the asset is not found, return an empty JSON object
        return nlohmann::json::object();
    }
}
