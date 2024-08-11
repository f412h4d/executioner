#ifndef PRICE_SESSION_HPP
#define PRICE_SESSION_HPP

#include "session.hpp"

class price_session : public session {
public:
    price_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams& api_params,
                  std::shared_ptr<double> calc_price, std::mutex &mutex)
            : session(ioc, ctx, api_params), calculated_price_(calc_price), price_mutex_(mutex) {}

    void on_handshake(boost::system::error_code ec) override {
        if (ec)
            return fail(ec, "handshake");

        std::cout << "Price session handshake completed" << std::endl;
        send_subscribe_message();
    }

    void send_subscribe_message() override {
        // Simplified subscription to a specific ticker stream
        nlohmann::json j;
        j["id"] = "price-subscribe-id";
        j["method"] = "SUBSCRIBE";
        j["params"] = {"ethusdt@ticker"};

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
        std::cout << "Price Update Received: " << message << std::endl;

        auto json_message = nlohmann::json::parse(message);

        if (json_message.contains("p")) {
            double price = std::stod(json_message["p"].get<std::string>());
            double calc_price = price * 0.95;

            {
                std::lock_guard<std::mutex> lock(price_mutex_);
                *calculated_price_ = calc_price;
            }

            std::cout << "Calculated Price (95%): " << calc_price << std::endl;
        }

        buffer_.consume(buffer_.size());

        ws_.async_read(
            buffer_,
            std::bind(
                &session::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void run(const std::string &host, const std::string &port) override {
        // Connect to the WebSocket endpoint
        host_ = host;

        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &session::on_resolve,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void start_keep_alive() override {
        keep_alive_thread_ = std::thread([this]() {
            while (keep_alive_running_) {
                std::this_thread::sleep_for(std::chrono::minutes(30));
                // Custom keep-alive logic for price_session
            }
        });
    }

private:
    std::shared_ptr<double> calculated_price_;
    std::mutex &price_mutex_;
};

#endif // PRICE_SESSION_HPP
