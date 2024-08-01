#ifndef EVENT_ORDER_UPDATE_SESSION_HPP
#define EVENT_ORDER_UPDATE_SESSION_HPP

#include "session.hpp"
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

class event_order_update_session : public session {
public:
    using session::session;

    void run(const std::string &host, const std::string &port) override {
        listen_key_ = get_listen_key(); // Ensure listen_key_ is defined in the base class
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
                std::bind(
                        &session::on_write,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        std::string message = boost::beast::buffers_to_string(buffer_.data());
        std::cout << "Order Update Received: " << message << std::endl;

        buffer_.consume(buffer_.size());

        ws_.async_read(
                buffer_,
                std::bind(
                        &session::on_read,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    void refresh_listen_key() {
        cpr::Response r = cpr::Put(cpr::Url{"https://fapi.binance.com/fapi/v1/listenKey"},
                                   cpr::Header{{"X-MBX-APIKEY", api_params_.apiKey}},
                                   cpr::Body{std::string{"listenKey="} + listen_key_});
        if (r.status_code != 200) {
            std::cerr << "Failed to refresh listenKey: " << r.status_code << " " << r.text << std::endl;
        }
    }

private:
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

};

#endif // EVENT_ORDER_UPDATE_SESSION_HPP
