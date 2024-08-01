#ifndef CUSTOM_SESSION_HPP
#define CUSTOM_SESSION_HPP

#include "session.hpp"

class price_session : public session {
public:
    using session::session;

    void on_handshake(boost::system::error_code ec) override {
        if (ec)
            return fail(ec, "handshake");

        send_custom_subscribe_message();
    }

    void send_custom_subscribe_message() {
        nlohmann::json j;
        j["id"] = "custom-id";
        j["method"] = "SUBSCRIBE";
        j["params"] = {"ethusdt@ticker"};

        ws_.async_write(
                boost::asio::buffer(j.dump()),
                [capture0 = shared_from_this()](auto && PH1, auto && PH2) { capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    }


    void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        std::string message = boost::beast::buffers_to_string(buffer_.data());
        std::cout << "Custom: " << message << std::endl;

        buffer_.consume(buffer_.size());

        ws_.async_read(
                buffer_,
                [capture0 = shared_from_this()](auto && PH1, auto && PH2) { capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    }
};

#endif // CUSTOM_SESSION_HPP
