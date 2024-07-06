#include "APIParams.h"

APIParams::APIParams(
        const std::string &apiKey,
        const std::string &apiSecret,
        const long &recvWindow,
        bool useTestnet
) :
        apiKey(apiKey),
        apiSecret(apiSecret),
        recvWindow(recvWindow),
        useTestnet(useTestnet) {}
