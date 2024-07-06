#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <iostream>
#include <string>

using TIME = std::chrono::steady_clock;
using TIMESTAMP = std::chrono::time_point<TIME>;
#define CENTENNIAL (TIME::now() + std::chrono::hours(100 * 365 * 24))

class TimedEventQueue {
private:
    struct Event {
        std::string label;
        std::function<void()> callback;
    };

    std::map<TIMESTAMP, Event> _ts2Val;
    std::map<std::string, TIMESTAMP> _val2Ts;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _exit = false;
    std::thread _thread;

    void run() {
        while (!_exit.load()) {
            std::unique_lock lock(_mutex);
            _cv.wait_until(lock, _ts2Val.begin()->first);

            if (_exit.load()) {
                break;
            }

            auto current_time = TIME::now();
            while (_ts2Val.begin()->first <= current_time) {
                auto event = _ts2Val.begin()->second;
                std::invoke(&TimedEventQueue::onTimestampExpire, this, _ts2Val.begin()->first, event.label);
                if (event.callback) {
                    event.callback();
                }
                _val2Ts.erase(event.label);
                _ts2Val.erase(_ts2Val.begin());
            }
        }
    }

protected:
    virtual void onTimestampExpire(const TIMESTAMP &timestamp, const std::string &label) = 0;

public:
    TimedEventQueue() {
        auto dummy_timestamp = CENTENNIAL;
        addEvent(dummy_timestamp, "", nullptr);
        _thread = std::thread(&TimedEventQueue::run, this);
    }

    virtual ~TimedEventQueue() { stop(); }

    TimedEventQueue(const TimedEventQueue &) = delete;

    TimedEventQueue &operator=(const TimedEventQueue &) = delete;

    void addEvent(const TIMESTAMP &timestamp, const std::string &label, const std::function<void()> &callback) {
        std::scoped_lock lock(_mutex);
        _ts2Val.emplace(timestamp, Event{label, callback});
        _val2Ts.emplace(label, timestamp);
        _cv.notify_one();
    }

    void removeEvent(const std::string &label) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _val2Ts.find(label); itr != _val2Ts.end()) {
            _ts2Val.erase(itr->second);
            _val2Ts.erase(itr);
        }
    }

    void removeEvent(const TIMESTAMP &timestamp) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _ts2Val.find(timestamp); itr != _ts2Val.end()) {
            _val2Ts.erase(itr->second.label);
            _ts2Val.erase(itr);
        }
    }

    void updateLabel(const TIMESTAMP &timestamp, const std::string &label) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _ts2Val.find(timestamp); _ts2Val.end() != itr) {
            auto nodeHandler = _val2Ts.extract(itr->second.label);
            nodeHandler.key() = label;
            _val2Ts.insert(std::move(nodeHandler));
            itr->second.label = label;
        }
        _cv.notify_one();
    }

    void updateTimestamp(const TIMESTAMP &timestamp, const std::string &label) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _val2Ts.find(label); _val2Ts.end() != itr) {
            auto nodeHandler = _ts2Val.extract(itr->second);
            nodeHandler.key() = timestamp;
            _ts2Val.insert(std::move(nodeHandler));
            itr->second = timestamp;
        }
        _cv.notify_one();
    }

    void stop() {
        if (_thread.joinable()) {
            _exit.store(true);
            _cv.notify_one();
            _thread.join();
        }
    }
};
