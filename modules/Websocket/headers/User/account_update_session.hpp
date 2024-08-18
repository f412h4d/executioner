#ifndef ACCOUNT_UPDATE_SESSION_HPP
#define ACCOUNT_UPDATE_SESSION_HPP

#include "spdlog/spdlog.h"
#include "user_session.hpp"
#include <nlohmann/json.hpp>

class account_update_session : public user_session {
public:
  account_update_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams &api_params)
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

    account_logger->info("Account Update: Handshake completed");
    send_account_update_request();
  }

  void send_account_update_request() {
    nlohmann::json j;
    j["method"] = "REQUEST";
    j["params"] = {listen_key_ + "@account", listen_key_ + "@balance"};
    j["id"] = 12;

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
    account_logger->info("Account Update Received: {}", message);

    try {
      auto json_msg = nlohmann::json::parse(message);
      if (json_msg.contains("e") && json_msg["e"] == "ACCOUNT_UPDATE") {
        auto update_data = json_msg["a"];
        process_account_update(update_data);
      }
    } catch (const std::exception &e) {
      account_logger->error("Error parsing JSON or updating account: {}", e.what());
    }

    buffer_.consume(buffer_.size());

    ws_.async_read(buffer_, [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

private:
  void process_account_update(const nlohmann::json &update_data) {
    auto account_logger = spdlog::get("account_logger");
    if (update_data.contains("B")) {
      for (const auto &balance : update_data["B"]) {
        std::string asset = balance["a"];
        double wallet_balance = std::stod(balance["wb"].get<std::string>());
        double cross_wallet_balance = std::stod(balance["cw"].get<std::string>());
        double balance_change = std::stod(balance["bc"].get<std::string>());

        account_logger->info("Asset: {}, Wallet Balance: {}, Cross Wallet Balance: {}, Balance Change: {}",
                             asset, wallet_balance, cross_wallet_balance, balance_change);
      }
    }

    if (update_data.contains("P")) {
      for (const auto &position : update_data["P"]) {
        std::string symbol = position["s"];
        double position_amount = std::stod(position["pa"].get<std::string>());
        double unrealized_pnl = std::stod(position["up"].get<std::string>());
        double entry_price = std::stod(position["ep"].get<std::string>());

        account_logger->info("Symbol: {}, Position Amount: {}, Unrealized PnL: {}, Entry Price: {}",
                             symbol, position_amount, unrealized_pnl, entry_price);
      }
    }
  }
};

#endif // ACCOUNT_UPDATE_SESSION_HPP

