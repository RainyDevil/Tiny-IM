#include "Session.h"
#include <iostream>

Session::Session(websocket::stream<tcp::socket> ws, std::shared_ptr<BusinessHandler> handler)
    :ws_(std::move(ws)), businessHandler_(handler) {}

void Session::start() {
    readMessage();
}

void Session::send(const Message& msg) {
    auto self(shared_from_this());
    boost::asio::post(ws_.get_executor(),
        [this, self, msg]() {
            bool writing = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!writing) {
                write();
            }
        }
    );
}

websocket::stream<tcp::socket>& Session::getSocket() {
    return ws_;
}

void Session::readMessage() {
    auto self(shared_from_this());
    ws_.async_read(
        buffer_,
        [this, self](beast::error_code ec, std::size_t length) {
            if (!ec) {
                std::string json_data = beast::buffers_to_string(buffer_.data());
                buffer_.consume(length); // Clear the buffer
                try {
                    Message msg = Message::fromJson(json_data);
                    businessHandler_->handleIncomingMessage(self, msg);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse message: " << e.what() << std::endl;
                }
                if (ws_.is_open()) {
                    readMessage(); // Read the next message
                }
            } else {
                std::cerr << "WebSocket read error: " << ec.message() << std::endl;
                ws_.close(websocket::close_code::normal);
            }
        }
    );
}
void Session::close() {
    auto self(shared_from_this());
    ws_.async_close(websocket::close_code::normal,
        [this,self](beast::error_code ec) {
            if (ec) {
                std::cerr << "WebSocket close error: " << ec.message() << std::endl;
            }
            businessHandler_->handleDisconnection(self);
        }
    );
}
void Session::write() {
    auto self(shared_from_this());
    const Message& msg = write_msgs_.front();
    std::string json_data = msg.toJson();

    ws_.async_write(
        boost::asio::buffer(json_data),
        [this, self](beast::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                write_msgs_.pop_front();
                if (!write_msgs_.empty()) {
                    write();
                }
            } else {
                std::cerr << "WebSocket write error: " << ec.message() << std::endl;
                ws_.close(websocket::close_code::normal);
            }
        }
    );
}
