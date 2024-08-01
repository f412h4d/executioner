#ifndef PRICE_SESSION_HPP
#define PRICE_SESSION_HPP

#include "session.hpp"

class price_session : public session {
public:
    using session::session;

    void on_handshake(boost::system::error_code ec) override {
        if (ec)
            return fail(ec, "handshake");

        std::cout << "Price session handshake completed" << std::endl;
        send_subscribe_message();
    }

    void send_subscribe_message() override {
        nlohmann::json j;
        j["id"] = "price-subscribe-id";
        j["method"] = "SUBSCRIBE";
        j["params"] = {"btcusdt@ticker"};

        ws_.async_write(
                boost::asio::buffer(j.dump()),
                std::bind(
                        &session::on_write,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }
};

#endif // PRICE_SESSION_HPP
