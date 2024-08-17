#ifndef BALANCE_SESSION_HPP
#define BALANCE_SESSION_HPP

#include "spdlog/spdlog.h"
#include "user_session.hpp"
#include "utils.h"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

class Balance {
public:
  double balance = 0.0;
  double crossWalletBalance = 0.0;
  double availableBalance = 0.0;
  double maxWithdrawAmount = 0.0;
  bool marginAvailable = false;
};

class balance_session : public user_session {
public:
  balance_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams &api_params,
                  std::shared_ptr<Balance> balance, std::mutex &mutex)
      : user_session(ioc, ctx, api_params), balance_(balance), balance_mutex_(mutex) {}

  void run(const std::string &host, const std::string &port) override { user_session::run(host, port); }

  void on_handshake(boost::system::error_code ec) override {
    auto balance_logger = spdlog::get("balance_logger");
    if (ec)
      return fail(ec, "handshake");

    balance_logger->info("Balance Session: Handshake completed");
    request_balance();
  }

  void request_balance() {
    auto balance_logger = spdlog::get("balance_logger");

    std::string apiKey = api_params_.apiKey;
    long timestamp = Utils::getCurrentTimestamp();
    std::string recvWindow = "5000";

    std::string data = "apiKey=" + apiKey + "&timestamp=" + std::to_string(timestamp) + "&recvWindow=" + recvWindow;
    std::string signature = Utils::HMAC_SHA256(api_params_.apiSecret, data);

    nlohmann::json request;
    request["id"] = Utils::generate_uuid();
    request["method"] = "v2/account.balance";
    request["params"]["apiKey"] = apiKey;
    request["params"]["timestamp"] = timestamp;
    request["params"]["recvWindow"] = recvWindow;
    request["params"]["signature"] = signature;

    ws_.async_write(boost::asio::buffer(request.dump()), [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
    auto balance_logger = spdlog::get("balance_logger");
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "read");

    std::string message = boost::beast::buffers_to_string(buffer_.data());
    balance_logger->info("Balance Update Received: {}", message);

    try {
      auto json_msg = nlohmann::json::parse(message);
      if (json_msg.contains("result")) {
        auto result = json_msg["result"];
        if (!result.empty()) {
          auto account_data = result[0]; // Assuming the first account entry is the one we need
          double balance = std::stod(account_data["balance"].get<std::string>());
          double crossWalletBalance = std::stod(account_data["crossWalletBalance"].get<std::string>());
          double availableBalance = std::stod(account_data["availableBalance"].get<std::string>());
          double maxWithdrawAmount = std::stod(account_data["maxWithdrawAmount"].get<std::string>());
          bool marginAvailable = account_data["marginAvailable"].get<bool>();

          {
            std::lock_guard<std::mutex> lock(balance_mutex_);
            balance_->balance = balance;
            balance_->crossWalletBalance = crossWalletBalance;
            balance_->availableBalance = availableBalance;
            balance_->maxWithdrawAmount = maxWithdrawAmount;
            balance_->marginAvailable = marginAvailable;
          }

          balance_logger->info("Balance: {}", balance);
          balance_logger->info("Cross Wallet Balance: {}", crossWalletBalance);
          balance_logger->info("Available Balance: {}", availableBalance);
          balance_logger->info("Max Withdraw Amount: {}", maxWithdrawAmount);
          balance_logger->info("Margin Available: {}", marginAvailable);
        }
      }
    } catch (const std::exception &e) {
      balance_logger->error("Error parsing JSON or updating balance: {}", e.what());
    }

    buffer_.consume(buffer_.size());

    ws_.async_read(buffer_, [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

private:
  std::shared_ptr<Balance> balance_;
  std::mutex &balance_mutex_;
};

#endif // BALANCE_SESSION_HPP
