#ifndef PRICE_SESSION_HPP
#define PRICE_SESSION_HPP

#include "market_session.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <mutex>

class price_session : public market_session {
public:
    price_session(boost::asio::io_context &ioc, ssl::context &ctx,
                  std::shared_ptr<double> calc_price, std::mutex &mutex)
        : market_session(ioc, ctx), calculated_price_(calc_price),
          price_mutex_(mutex) {}

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
            [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
                capture0->on_write(std::forward<decltype(PH1)>(PH1),
                                   std::forward<decltype(PH2)>(PH2));
            });
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        std::string message = boost::beast::buffers_to_string(buffer_.data());
        std::cout << "Custom: " << message << std::endl;

        try {
            // Parse the incoming JSON message
            auto json_msg = nlohmann::json::parse(message);

            // Extract the current price
            if (json_msg.contains("c")) {
                double current_price = std::stod(json_msg["c"].get<std::string>());

                // Calculate 90% of the current price
                double new_calculated_price = current_price * 0.9;

                // Safely update the shared calculated_price
                {
                    std::lock_guard<std::mutex> lock(price_mutex_);
                    *calculated_price_ = new_calculated_price;
                }

                std::cout << "Updated Calculated Price (90%): " << new_calculated_price << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error parsing JSON or updating price: " << e.what() << std::endl;
        }

        buffer_.consume(buffer_.size());

        ws_.async_read(
            buffer_,
            [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
                capture0->on_read(std::forward<decltype(PH1)>(PH1),
                                  std::forward<decltype(PH2)>(PH2));
            });
    }

private:
    std::shared_ptr<double> calculated_price_;
    std::mutex &price_mutex_;
};

#endif // PRICE_SESSION_HPP
