#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <memory>
#include <set>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include "Session.h"
#include "Log.h"
using boost::asio::ip::tcp;
namespace beast = boost::beast;             // from <boost/beast.hpp>
namespace http = beast::http;               // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;     // from <boost/beast/websocket.hpp>

class Server {
public:
    Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);

private:
    void accept();
    std::shared_ptr<BusinessHandler> shared_handler_;
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<Session>> sessions_;
};

#endif // SERVER_H
