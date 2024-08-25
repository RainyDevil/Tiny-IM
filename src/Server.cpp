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
                std::cout << "WebSocket session created." << std::endl;
                ws->async_accept(
                    [this, ws](boost::system::error_code ec) {
                        if (!ec) {
                            auto session = std::make_shared<Session>(std::move(*ws),shared_handler_);
                            sessions_.insert(session);
                            std::cout << "Insert a WebSocket session." << std::endl;
                            session->start();
                        } else {
                            ws->close(websocket::close_code::protocol_error);
                            std::cerr << "WebSocket handshake failed: " << ec.message() << std::endl;
                        }
                    }
                );
            }
            accept(); // 接受下一个连接
        }
    );
}
