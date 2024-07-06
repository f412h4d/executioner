#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "utils.h"
#include "signaling.h"
#include "APIParams.h"

#include "modules/TimedEventQueue/headers/TimedEventQueue.hpp"

class MyTimedEventQueue : public TimedEventQueue<int> {
protected:
    void onTimestampExpire(const TIMESTAMP &timestamp, const int &value) override {
        std::cout << "Timestamp expired: "
                  << std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count()
                  << " with value: " << value << std::endl;
    }
};


int main() {
    MyTimedEventQueue myTimedEventQueue;

    auto now = TIME::now();
    myTimedEventQueue.addEvent(now + std::chrono::seconds(3), 1, []() {
        std::cout << "Callback for event 1 after 3 seconds" << std::endl;
    });
    myTimedEventQueue.addEvent(now + std::chrono::seconds(10), 2, []() {
        std::cout << "Callback for event 2 after 10 seconds" << std::endl;
    });
    myTimedEventQueue.addEvent(now + std::chrono::seconds(2), 3, []() {
        std::cout << "Callback for event 3 after 2 seconds" << std::endl;
    });
    myTimedEventQueue.addEvent(now + std::chrono::seconds(4), 4, []() {
        std::cout << "Callback for event 4 after 4 seconds" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(20));                          // sleep for 6 seconds
    myTimedEventQueue.stop();
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
