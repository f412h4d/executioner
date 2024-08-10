#ifndef EVENT_ORDER_UPDATE_SESSION_HPP
#define EVENT_ORDER_UPDATE_SESSION_HPP

#include "session.hpp"
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

class event_order_update_session : public session {
public:
    event_order_update_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams& api_params,
                               std::shared_ptr<double> calc_price, std::mutex &mutex)
            : session(ioc, ctx, api_params), calculated_price_(calc_price), price_mutex_(mutex) {}

    void run(const std::string &host, const std::string &port) override {
        listen_key_ = get_listen_key(); 
        if (listen_key_.empty()) {
            std::cerr << "Failed to get listenKey" << std::endl;
            return;
        }
        session::run(host, port);
    }

    void on_handshake(boost::system::error_code ec) override {
        if (ec)
            return fail(ec, "handshake");

        std::cout << "Event Order Update: Handshake completed" << std::endl;
        send_subscribe_message();
    }

    void send_subscribe_message() override {
        nlohmann::json j;
        j["id"] = "order-update-id";
        j["method"] = "SUBSCRIBE";
        j["params"] = {"btcusdt@orderUpdate"};

        ws_.async_write(
                boost::asio::buffer(j.dump()),
                [capture0 = shared_from_this()](auto && PH1, auto && PH2) { capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        std::string message = boost::beast::buffers_to_string(buffer_.data());
        std::cout << "Order Update Received: " << message << std::endl;

        // Access the shared calculated price
        double calc_price;
        {
            std::lock_guard<std::mutex> lock(price_mutex_);
            calc_price = *calculated_price_;
        }

        std::cout << "Current Calculated Price (95%): " << calc_price << std::endl;

        buffer_.consume(buffer_.size());

        ws_.async_read(
                buffer_,
                [capture0 = shared_from_this()](auto && PH1, auto && PH2) { capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    }

private:
    std::shared_ptr<double> calculated_price_;
    std::mutex &price_mutex_;

    std::string get_listen_key() {
        cpr::Response r = cpr::Post(cpr::Url{"https://fapi.binance.com/fapi/v1/listenKey"},
                                    cpr::Header{{"X-MBX-APIKEY", api_params_.apiKey}});
        if (r.status_code == 200) {
            auto json = nlohmann::json::parse(r.text);
            return json["listenKey"].get<std::string>();
        } else {
            std::cerr << "Failed to get listenKey: " << r.status_code << " " << r.text << std::endl;
            return "";
        }
    }

    void refresh_listen_key() {
        cpr::Response r = cpr::Put(cpr::Url{"https://fapi.binance.com/fapi/v1/listenKey"},
                                   cpr::Header{{"X-MBX-APIKEY", api_params_.apiKey}},
                                   cpr::Body{std::string{"listenKey="} + listen_key_});
        if (r.status_code != 200) {
            std::cerr << "Failed to refresh listenKey: " << r.status_code << " " << r.text << std::endl;
        }
    }

    std::thread keep_alive_thread_;
    std::atomic<bool> keep_alive_running_{true};
};

#endif // EVENT_ORDER_UPDATE_SESSION_HPP
