#include <iostream>
#include <cpr/cpr.h>

#include "utils.h"
#include "signaling.h"
#include "APIParams.h"
#include "modules/TimedEventQueue/headers/SignalQueue.h"
#include "margin.h"

// Function to print all elements in the map
void printMapElements(const std::map<std::string, std::string>& env) {
    for (const auto& pair : env) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
}

int main() {
    std::string exePath = Utils::getExecutablePath();
    std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
    std::string envFilePath = exeDir + "/../.env";
    std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

    printMapElements(env);

    bool useTestnet = (env["TESTNET"] == "TRUE");
    std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
    std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

    std::cout << env["TESTNET"] << std::endl;
    std::cout << env["API_KEY"] << std::endl;
    std::cout << env["API_SECRET"] << std::endl;

    APIParams testApiParams(
            env["TESTNET_API_KEY"],
            env["TESTNET_API_SECRET"],
            5000,
            true
    );
    Margin::getPositions(testApiParams, "BTCUSDT");

    APIParams apiParams(
            apiKey,
            apiSecret,
            5000,
            env["TESTNET"] == "TRUE"
    );
    Margin::getPositions(apiParams, "BTCUSDT");
//    Signaling::mockSignal(apiParams);

    return 0;
}
