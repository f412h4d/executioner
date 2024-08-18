#ifndef EVENT_ORDER_UPDATE_SESSION_HPP
#define EVENT_ORDER_UPDATE_SESSION_HPP

#include "spdlog/spdlog.h"
#include "user_session.hpp"
#include <nlohmann/json.hpp>

class balance_session : public user_session {
public:
  balance_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams &api_params)
      : user_session(ioc, ctx, api_params) {}

  void run(const std::string &host, const std::string &port) override {
    listen_key_ = get_listen_key();
    if (listen_key_.empty()) {
      auto account_logger = spdlog::get("account_logger");
      account_logger->error("Failed to get listenKey");
      return;
    }
    user_session::run(host, port);
  }

  void on_handshake(boost::system::error_code ec) override {
    auto account_logger = spdlog::get("account_logger");
    if (ec)
      return fail(ec, "handshake");

    account_logger->info("Event Order Update: Handshake completed");
    send_subscribe_message();
  }

  void send_subscribe_message() override {
    nlohmann::json j;
    j["id"] = "balance-update-id";
    j["method"] = "SUBSCRIBE";
    j["params"] = {listen_key_ + "@balanceUpdate"};

    ws_.async_write(boost::asio::buffer(j.dump()), [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
    auto account_logger = spdlog::get("account_logger");
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "read");

    std::string message = boost::beast::buffers_to_string(buffer_.data());
    account_logger->info("Balance Received: {}", message);

    buffer_.consume(buffer_.size());

    ws_.async_read(buffer_, [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }
};

#endif // EVENT_ORDER_UPDATE_SESSION_HPP
