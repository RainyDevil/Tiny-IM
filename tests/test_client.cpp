#include <iostream>
#include <boost/asio.hpp>
#include <stdio.h>
#include "Message.h"

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io_context, const std::string& server, const std::string& port)
        : socket_(io_context) {
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server, port);
        boost::asio::connect(socket_, endpoints);
    }

    void sendMessage(const Message& msg) {
        boost::asio::write(socket_, boost::asio::buffer(msg.data(), msg.length()));
    }
    void receiveMessage() {
        try {
            bool receive_flag = false;
            while (receive_flag) {
                // 接收消息头
                char header[Message::header_length];
                boost::asio::read(socket_, boost::asio::buffer(header, Message::header_length));

                // 创建一个Message对象并解析头部
                Message msg;
                std::memcpy(msg.data(), header, Message::header_length);
                if (!msg.decodeHeader()) {
                    std::cerr << "Failed to decode header." << std::endl;
                    return;
                }

                // 根据头中长度信息接收消息体
                boost::asio::read(socket_, boost::asio::buffer(msg.body(), msg.bodyLength()));

                // 打印收到的消息
                std::cout << "Received message: " << msg.data() << std::endl;
                receive_flag = true;
            }
        } catch (std::exception& e) {
            std::cerr << "Receive error: " << e.what() << std::endl;
        }
    }
    void close() {
        socket_.close();
    }

private:
    tcp::socket socket_;
};

int main() {
    try {
        boost::asio::io_context io_context;

        Client client(io_context, "localhost", "8080");

        Message msg;
        msg.constructMessage(Message::LIST_FRIENDS, 12334, 67891, "Hello, this is a test message");
        Message msg_resolve = Message::fromData(msg.data());
        std::cout << msg_resolve.getMessageType() << std::endl;
        std::cout << msg_resolve.getMessageId() << std::endl;
        std::cout << msg_resolve.getSessionId() << std::endl;
        std::cout << msg_resolve.getContent()<<std::endl;
        int loop = 100;
        while(loop--)
        {
            client.sendMessage(msg);
            std::cout << "Message sent to server." << std::endl;
            client.receiveMessage();
        }
        client.close();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
