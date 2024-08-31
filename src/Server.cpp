#include "Server.h"
#include <iostream>


Server::Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint) {
    shared_handler_ = std::make_shared<BusinessHandler>();    
    accept();
}

void Server::accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
                LOG_INFO("WebSocket session created.");
                ws->async_accept(
                    [this, ws](boost::system::error_code ec) {
                        if (!ec) {
                            auto session = std::make_shared<Session>(std::move(*ws),shared_handler_,
                            [this](std::shared_ptr<Session> s) { removeSession(s);});
                            sessions_.insert(session);
                            LOG_INFO("Insert a WebSocket session.");
                            LOG_INFO("Insertion count = {}", sessions_.size());
                            session->start();
                        } else {
                            ws->close(websocket::close_code::protocol_error);
                            LOG_ERROR("WebSocket handshake failed: {}", ec.message());
                        }
                    }
                );
            }
            accept(); // 接受下一个连接
        }
    );
}
void Server::removeSession(std::shared_ptr<Session> session) {
    sessions_.erase(session);
    LOG_INFO("Session removed. Current session count: {}", sessions_.size());
}