#include <iostream>
#include <cpr/cpr.h>

#include "utils.h"
#include "margin.h"
#include "signaling.h"
#include "APIParams.h"

int main() {
    std::string exePath = Utils::getExecutablePath();
    std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
    std::string envFilePath = exeDir + "/../.env";
    std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

    std::cout << "ENV VARIABLES:" << std::endl;
    std::cout << "________________________" << std::endl;
    Utils::printMapElements(env);
    std::cout << "________________________" << std::endl << std::endl;

    bool useTestnet = (env["TESTNET"] == "TRUE");
    std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
    std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

    APIParams apiParams(
            apiKey,
            apiSecret,
            5000,
            env["TESTNET"] == "TRUE"
    );
    Margin::setLeverage(apiParams, "BTCUSDT", 1);
    Signaling::init(apiParams);
}
