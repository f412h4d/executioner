#ifndef USER_SESSION_HPP
#define USER_SESSION_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <iostream>
#include "APIParams.h"
#include <thread>
#include <atomic>

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace websocket = boost::beast::websocket;

class user_session : public std::enable_shared_from_this<user_session> {
protected:
    tcp::resolver resolver_;
    websocket::stream<ssl::stream<tcp::socket>> ws_;
    boost::beast::multi_buffer buffer_;
    std::string host_;
    APIParams api_params_;
    std::string listen_key_;

    std::thread keep_alive_thread_;
    std::atomic<bool> keep_alive_running_{true};

public:
    explicit user_session(boost::asio::io_context &ioc, ssl::context &ctx, const APIParams& api_params)
            : resolver_(ioc), ws_(ioc, ctx), api_params_(api_params) {}

    virtual ~user_session() {
        stop_keep_alive();
    }

    virtual void run(const std::string &host, const std::string &port) {
        host_ = host;
        resolver_.async_resolve(
                host,
                port,
                std::bind(
                        &user_session::on_resolve,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    virtual void on_resolve(boost::system::error_code ec, const tcp::resolver::results_type &results) {
        if (ec)
            return fail(ec, "resolve");

        boost::asio::async_connect(
                ws_.next_layer().next_layer(),
                results.begin(),
                results.end(),
                std::bind(
                        &user_session::on_connect,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    virtual void on_connect(boost::system::error_code ec) {
        if (ec)
            return fail(ec, "connect");

        ws_.next_layer().async_handshake(
                ssl::stream_base::client,
                std::bind(
                        &user_session::on_ssl_handshake,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    virtual void on_ssl_handshake(boost::system::error_code ec) {
        if (ec)
            return fail(ec, "ssl_handshake");

        std::string websocket_url = "/ws/" + listen_key_;
        ws_.async_handshake(host_, websocket_url,
                            std::bind(
                                    &user_session::on_handshake,
                                    shared_from_this(),
                                    std::placeholders::_1));
    }

    virtual void on_handshake(boost::system::error_code ec) {
        if (ec)
            return fail(ec, "handshake");

        send_subscribe_message();
    }

    virtual void send_subscribe_message() {
        nlohmann::json j;
        j["id"] = "9d32157c-a556-4d27-9866-66760a174b57";
        j["method"] = "SUBSCRIBE";
        j["params"] = {"btcusdt@ticker"};

        ws_.async_write(
                boost::asio::buffer(j.dump()),
                std::bind(
                        &user_session::on_write,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    virtual void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        ws_.async_read(
                buffer_,
                std::bind(
                        &user_session::on_read,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    virtual void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        std::string message = boost::beast::buffers_to_string(buffer_.data());
        std::cout << "Received: " << message << std::endl;

        buffer_.consume(buffer_.size());

        ws_.async_read(
                buffer_,
                std::bind(
                        &user_session::on_read,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    virtual void start_keep_alive() {
        keep_alive_thread_ = std::thread();
        // ! FIXME
// [this]() {
//             while (keep_alive_running_) {
//                 // std::this_thread::sleep_for(std::chrono::minutes(30));
//                 // Logic for keeping the connection alive or refreshing it
//             }
//         }
    }

    void stop_keep_alive() {
        keep_alive_running_ = false;
        if (keep_alive_thread_.joinable()) {
            keep_alive_thread_.join();
        }
    }

    static void fail(boost::system::error_code ec, char const *what) {
        std::cerr << what << ": " << ec.message() << "\n";
    }
};

#endif // USER_SESSION_HPP
