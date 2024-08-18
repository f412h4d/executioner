#include "APIParams.h"

APIParams::APIParams(const std::string &apiKey, const std::string &apiSecret, const long &recvWindow, bool useTestnet,
                     std::string host)
    : apiKey(apiKey), apiSecret(apiSecret), recvWindow(recvWindow), useTestnet(useTestnet), host(host) {}
