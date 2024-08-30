#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <deque>
#include <string>
#include "Message.h"
#include "BusinessHandler.h"
#include "Log.h"
class BusinessHandler;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(websocket::stream<tcp::socket> ws,std::shared_ptr<BusinessHandler> handler);

    void start();
    void send(const Message& msg);
    void close();
    websocket::stream<tcp::socket>& getSocket();

private:
    void readMessage();
    void write();
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::shared_ptr<BusinessHandler> businessHandler_;
    Message read_msg_;
    std::deque<Message> write_msgs_;
};

