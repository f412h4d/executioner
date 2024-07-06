#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "utils.h"
#include "signaling.h"
#include "APIParams.h"
#include "modules/TimedEventQueue/headers/SignalQueue.h"


int main() {
    SignalQueue signalQueue;

    auto now = TIME::now();
    signalQueue.addEvent(now + std::chrono::seconds(3), "First event after 3 secs", []() {
        std::cout << "Callback for event 1 after 3 seconds" << std::endl;
    });
    signalQueue.addEvent(now + std::chrono::seconds(10), "Second event after 10 secs", []() {
        std::cout << "Callback for event 2 after 10 seconds" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(20));                          // sleep for 6 seconds
    signalQueue.stop();
    std::string exePath = Utils::getExecutablePath();
    std::string exeDir = exePath.substr(0, exePath.find_last_of('/'));
    std::string envFilePath = exeDir + "/../.env";
    std::map<std::string, std::string> env = Utils::loadEnvFile(envFilePath);

    bool useTestnet = (env["TESTNET"] == "TRUE");
    std::string apiKey = useTestnet ? env["TESTNET_API_KEY"] : env["API_KEY"];
    std::string apiSecret = useTestnet ? env["TESTNET_API_SECRET"] : env["API_SECRET"];

    APIParams apiParams(
            apiKey,
            apiSecret,
            5000,
            true
    );

    Signaling::mockSignal(exeDir + "/../signals.txt", apiParams);

    return 0;
}
