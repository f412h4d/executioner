#include <iostream>
#include <cpr/cpr.h>

#include "utils.h"
#include "signaling.h"
#include "APIParams.h"
#include "modules/TimedEventQueue/headers/SignalQueue.h"
#include "margin.h"


int main() {
    std::string exePath = Utils::getExecutablePath();
    std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
    std::string envFilePath = exeDir + "/../.env";
    std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

    bool useTestnet = (env["TESTNET"] == "TRUE");
    std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
    std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

    std::cout << env["API_KEY"] << std::endl;
    std::cout << env["TESTNET"] << std::endl;

    APIParams apiParams(
            apiKey,
            apiSecret,
            5000,
            env["TESTNET"] == "TRUE"
    );
    Signaling::mockSignal(apiParams);

    return 0;
}
