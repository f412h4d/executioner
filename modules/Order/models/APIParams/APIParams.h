#ifndef API_PARAMS_H
#define API_PARAMS_H

#include <string>

class APIParams {
public:
    std::string apiKey;
    std::string apiSecret;
    long recvWindow;
    bool useTestnet;

    APIParams(
            const std::string &apiKey,
            const std::string &apiSecret,
            const long &recvWindow,
            bool useTestnet
    );
};

#endif // API_PARAMS_H
