#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <iostream>

using TIME = std::chrono::steady_clock;
using TIMESTAMP = std::chrono::time_point<TIME>;
#define CENTENNIAL (TIME::now() + std::chrono::hours(100 * 365 * 24))

template<typename T>
class TimedEventQueue {
private:
    struct Event {
        T value;
        std::function<void()> callback;
    };

    std::map<TIMESTAMP, Event> _ts2Val;
    std::map<T, TIMESTAMP> _val2Ts;
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
                std::invoke(&TimedEventQueue<T>::onTimestampExpire, this, _ts2Val.begin()->first, event.value);
                if (event.callback) {
                    event.callback();
                }
                _val2Ts.erase(event.value);
                _ts2Val.erase(_ts2Val.begin());
            }
        }
    }

protected:
    virtual void onTimestampExpire(const TIMESTAMP &timestamp, const T &value) = 0;

public:
    TimedEventQueue() {
        auto dummy_timestamp = CENTENNIAL;
        addEvent(dummy_timestamp, T(), nullptr);
        _thread = std::thread(&TimedEventQueue::run, this);
    }

    virtual ~TimedEventQueue() { stop(); }

    TimedEventQueue(const TimedEventQueue &) = delete;

    TimedEventQueue &operator=(const TimedEventQueue &) = delete;

    void addEvent(const TIMESTAMP &timestamp, const T &value, const std::function<void()> &callback) {
        std::scoped_lock lock(_mutex);
        _ts2Val.emplace(timestamp, Event{value, callback});
        _val2Ts.emplace(value, timestamp);
        _cv.notify_one();
    }

    void removeEvent(const T &value) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _val2Ts.find(value); itr != _val2Ts.end()) {
            _ts2Val.erase(itr->second);
            _val2Ts.erase(itr);
        }
    }

    void removeEvent(const TIMESTAMP &timestamp) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _ts2Val.find(timestamp); itr != _ts2Val.end()) {
            _val2Ts.erase(itr->second.value);
            _ts2Val.erase(itr);
        }
    }

    void updateValue(const TIMESTAMP &timestamp, const T &value) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _ts2Val.find(timestamp); _ts2Val.end() != itr) {
            auto nodeHandler = _val2Ts.extract(itr->second.value);
            nodeHandler.key() = value;
            _val2Ts.insert(std::move(nodeHandler));
            itr->second.value = value;
        }
        _cv.notify_one();
    }

    void updateTimestamp(const TIMESTAMP &timestamp, const T &value) {
        std::scoped_lock lock(_mutex);
        if (auto itr = _val2Ts.find(value); _val2Ts.end() != itr) {
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
