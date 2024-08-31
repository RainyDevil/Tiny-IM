#include "Session.h"
#include <iostream>

Session::Session(websocket::stream<tcp::socket> ws, std::shared_ptr<BusinessHandler> handler,
    std::function<void(std::shared_ptr<Session>)> on_close)
    :ws_(std::move(ws)), businessHandler_(handler), on_close_(on_close) {}

void Session::start() {
    readMessage();
}
/// @brief 客户端断开连接，服务器要做清理从set中移除Session
void Session::stop() {
    boost::system::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    if (on_close_) {
        on_close_(shared_from_this());  // 调用关闭回调
    }
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
                    LOG_INFO("JSON meta data : {}", json_data); 
                    Message msg = Message::fromJson(json_data);
                    businessHandler_->handleIncomingMessage(self, msg);
                } catch (const std::exception& e) {
                    LOG_ERROR("Failed to parse message: {}", e.what());
                }
                if (ws_.is_open()) {
                    readMessage(); // Read the next message
                }
            } else {
                stop();
                LOG_ERROR("WebSocket read error: {}", ec.message());
            }
        }
    );
}
/// @brief 服务器主动断开连接
void Session::close() {
    auto self(shared_from_this());
    ws_.async_close(websocket::close_code::normal,
        [this,self](beast::error_code ec) {
            if (ec) {
                LOG_ERROR("WebSocket close error: {}", ec.message());
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
                    if(ws_.is_open()){
                        write();
                    }
                }
            } else {
                stop();
                LOG_ERROR("WebSocket write error: {}", ec.message());
            }
        }
    );
}
