#pragma once

#include "./TimedEventQueue.hpp"

class SignalQueue : public TimedEventQueue {
protected:
    void onTimestampExpire(const TIMESTAMP &timestamp, const std::string &label) override {
        std::cout << "Timestamp expired: "
                  << std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count()
                  << " with label: " << label << std::endl;
    }
};
