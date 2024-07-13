#include "../headers/margin.h"
#include "../../Utils/headers/utils.h"
#include "cpr/cpr.h"
#include <iostream>
#include <chrono>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace Margin {
    string getTimestamp() {
        long long ms_since_epoch = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        return to_string(ms_since_epoch);
    }

    string encryptWithHMAC(const string &key, const string &data) {
        unsigned char *result;
        static char res_hexstring[64];
        int result_len = 32;
        string signature;

        result = HMAC(EVP_sha256(), key.c_str(), key.length(), reinterpret_cast<const unsigned char *>(data.c_str()), data.length(), NULL, NULL);
        for (int i = 0; i < result_len; i++) {
            sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
        }

        for (int i = 0; i < 64; i++) {
            signature += res_hexstring[i];
        }

        return signature;
    }

    string getSignature(const string &query) {
        const string apiSecret = getenv("BINANCE_API_SECRET");
        string signature = encryptWithHMAC(apiSecret, query);
        return "&signature=" + signature;
    }

    string joinQueryParameters(const unordered_map<string, string> &parameters) {
        string queryString;
        for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
            if (it != parameters.cbegin()) {
                queryString += '&';
            }
            queryString += it->first + '=' + it->second;
        }
        return queryString;
    }

    double getPrice(const APIParams &apiParams, const string &symbol) {
        string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        string apiCall = "fapi/v1/ticker/price";
        string url = baseUrl + "/" + apiCall + "?symbol=" + symbol;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        cout << "Response Code: " << r.status_code << endl;
        cout << "Response Text: " << r.text << endl;

        return stod(nlohmann::json::parse(r.text)["price"].get<string>());
    }

    nlohmann::json getPositions(const APIParams &apiParams, const string &symbol) {
        string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        string apiCall = "fapi/v2/positionRisk";

        unordered_map<string, string> parameters;
        parameters["timestamp"] = getTimestamp();
        if (!symbol.empty()) {
            parameters["symbol"] = symbol;
        }
        parameters["recvWindow"] = to_string(apiParams.recvWindow);

        string queryString = joinQueryParameters(parameters);
        string signature = getSignature(queryString);
        string url = baseUrl + "/" + apiCall + "?" + queryString + signature;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        cout << "Response Code: " << r.status_code << endl;
        cout << "Response Text: " << r.text << endl;

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getOpenOrders(const APIParams &apiParams, const string &symbol) {
        string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        string apiCall = "fapi/v1/openOrders";

        unordered_map<string, string> parameters;
        parameters["timestamp"] = getTimestamp();
        if (!symbol.empty()) {
            parameters["symbol"] = symbol;
        }
        parameters["recvWindow"] = to_string(apiParams.recvWindow);

        string queryString = joinQueryParameters(parameters);
        string signature = getSignature(queryString);
        string url = baseUrl + "/" + apiCall + "?" + queryString + signature;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        cout << "Response Code: " << r.status_code << endl;
        cout << "Response Text: " << r.text << endl;

        return nlohmann::json::parse(r.text);
    }

    nlohmann::json getBalance(const APIParams &apiParams, const string &asset) {
        string baseUrl = apiParams.useTestnet ? "https://testnet.binancefuture.com" : "https://fapi.binance.com";
        string apiCall = "fapi/v2/account";

        unordered_map<string, string> parameters;
        parameters["timestamp"] = getTimestamp();
        parameters["recvWindow"] = to_string(apiParams.recvWindow);

        string queryString = joinQueryParameters(parameters);
        string signature = getSignature(queryString);
        string url = baseUrl + "/" + apiCall + "?" + queryString + signature;

        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"X-MBX-APIKEY", apiParams.apiKey}});
        cout << "Response Code: " << r.status_code << endl;
        cout << "Response Text: " << r.text << endl;

        nlohmann::json jsonResponse = nlohmann::json::parse(r.text);

        for (const auto &balance : jsonResponse["assets"]) {
            if (balance["asset"] == asset) {
                return balance;
            }
        }

        return nlohmann::json::object();
    }
}
