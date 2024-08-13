#ifndef EVENT_ORDER_UPDATE_SESSION_HPP
#define EVENT_ORDER_UPDATE_SESSION_HPP

#include "user_session.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

class event_order_update_session : public user_session {
public:
  event_order_update_session(boost::asio::io_context &ioc, ssl::context &ctx,
                             const APIParams &api_params,
                             std::shared_ptr<double> calc_price,
                             std::mutex &mutex)
      : user_session(ioc, ctx, api_params), calculated_price_(calc_price),
        price_mutex_(mutex) {}

  void run(const std::string &host, const std::string &port) override {
    listen_key_ = get_listen_key();
    if (listen_key_.empty()) {
      std::cerr << "Failed to get listenKey" << std::endl;
      return;
    }
    user_session::run(host, port);
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

    ws_.async_write(boost::asio::buffer(j.dump()),
                    [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
                      capture0->on_write(std::forward<decltype(PH1)>(PH1),
                                         std::forward<decltype(PH2)>(PH2));
                    });
  }

  void on_read(boost::system::error_code ec,
               std::size_t bytes_transferred) override {
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "read");

    std::string message = boost::beast::buffers_to_string(buffer_.data());
    std::cout << "Order Update Received: " << message << std::endl;

    double calc_price;
    {
      std::lock_guard<std::mutex> lock(price_mutex_);
      calc_price = *calculated_price_;
    }

    std::cout << "Current Calculated Price (95%): " << calc_price << std::endl;

    buffer_.consume(buffer_.size());

    ws_.async_read(buffer_,
                   [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
                     capture0->on_read(std::forward<decltype(PH1)>(PH1),
                                       std::forward<decltype(PH2)>(PH2));
                   });
  }

private:
  std::shared_ptr<double> calculated_price_;
  std::mutex &price_mutex_;
};

#endif // EVENT_ORDER_UPDATE_SESSION_HPP
