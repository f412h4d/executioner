#ifndef MARGIN_SESSION_HPP
#define MARGIN_SESSION_HPP

#include "spdlog/spdlog.h"
#include "user_session.hpp"
#include "utils.h"
#include <nlohmann/json.hpp>

class margin_session : public user_session {
public:
  margin_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams &api_params)
      : user_session(ioc, ctx, api_params) {}

  void run(const std::string &host, const std::string &port) override {
    auto margin_logger = spdlog::get("margin_logger");
    margin_logger->info("Starting margin session with host: {} and port: {}", host, port);
    user_session::run(host, port);
  }

  void on_handshake(boost::system::error_code ec) override {
    auto margin_logger = spdlog::get("margin_logger");
    if (ec) {
      margin_logger->error("Handshake failed: {}", ec.message());
      return fail(ec, "handshake");
    }

    margin_logger->info("Margin Session: Handshake completed successfully.");
    request_margin();
  }

  void request_margin() {
    auto margin_logger = spdlog::get("margin_logger");

    std::string apiKey = api_params_.apiKey;
    long timestamp = Utils::getCurrentTimestamp();
    std::string recvWindow = "5000";

    margin_logger->info("Preparing request for margin status.");
    margin_logger->debug("API Key: {}", apiKey);
    margin_logger->debug("Timestamp: {}", timestamp);
    margin_logger->debug("Recv Window: {}", recvWindow);

    std::string data = "apiKey=" + apiKey + "&timestamp=" + std::to_string(timestamp) + "&recvWindow=" + recvWindow;
    std::string signature = Utils::HMAC_SHA256(api_params_.apiSecret, data);

    margin_logger->debug("Signature: {}", signature);

    nlohmann::json request;
    request["id"] = Utils::generate_uuid();
    request["method"] = "v2/account.status";
    request["params"]["apiKey"] = apiKey;
    request["params"]["timestamp"] = timestamp;
    request["params"]["recvWindow"] = recvWindow;
    request["params"]["signature"] = signature;

    margin_logger->info("Sending margin status request: {}", request.dump());

    ws_.async_write(boost::asio::buffer(request.dump()), [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_write(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) override {
    auto margin_logger = spdlog::get("margin_logger");
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      margin_logger->error("Read failed: {}", ec.message());
      return fail(ec, "read");
    }

    std::string message = boost::beast::buffers_to_string(buffer_.data());
    margin_logger->info("Margin Update Received: {}", message);

    // try {
    //   auto json_msg = nlohmann::json::parse(message);
    //   if (json_msg.contains("result")) {
    //     auto result = json_msg["result"];
    //     if (!result.empty()) {
    //       {
    //         std::lock_guard<std::mutex> lock(status_mutex_);
    //         margin_logger->info("Updating account status.");
    //         account_status_->totalInitialMargin = std::stod(result["totalInitialMargin"].get<std::string>());
    //         account_status_->totalMaintMargin = std::stod(result["totalMaintMargin"].get<std::string>());
    //         account_status_->totalWalletBalance = std::stod(result["totalWalletBalance"].get<std::string>());
    //         account_status_->totalUnrealizedProfit = std::stod(result["totalUnrealizedProfit"].get<std::string>());
    //         account_status_->totalMarginBalance = std::stod(result["totalMarginBalance"].get<std::string>());
    //         account_status_->totalPositionInitialMargin =
    //             std::stod(result["totalPositionInitialMargin"].get<std::string>());
    //         account_status_->totalOpenOrderInitialMargin =
    //             std::stod(result["totalOpenOrderInitialMargin"].get<std::string>());
    //         account_status_->totalCrossWalletBalance = std::stod(result["totalCrossWalletBalance"].get<std::string>());
    //         account_status_->totalCrossUnPnl = std::stod(result["totalCrossUnPnl"].get<std::string>());
    //         account_status_->availableBalance = std::stod(result["availableBalance"].get<std::string>());
    //         account_status_->maxWithdrawAmount = std::stod(result["maxWithdrawAmount"].get<std::string>());
    //       }
    //
    //       margin_logger->info("Total Initial Margin: {}", account_status_->totalInitialMargin);
    //       margin_logger->info("Total Maintenance Margin: {}", account_status_->totalMaintMargin);
    //       margin_logger->info("Total Wallet Balance: {}", account_status_->totalWalletBalance);
    //       margin_logger->info("Total Unrealized Profit: {}", account_status_->totalUnrealizedProfit);
    //       margin_logger->info("Total Margin Balance: {}", account_status_->totalMarginBalance);
    //     }
    //   } else {
    //     margin_logger->warn("Result not found in the received message.");
    //   }
    // } catch (const std::exception &e) {
    //   margin_logger->error("Error parsing JSON or updating margin: {}", e.what());
    // }

    buffer_.consume(buffer_.size());

    ws_.async_read(buffer_, [capture0 = shared_from_this()](auto &&PH1, auto &&PH2) {
      capture0->on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
  }
};

#endif // MARGIN_SESSION_HPP
