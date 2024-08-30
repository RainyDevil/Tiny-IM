#include <iostream>
#include "Server.h"
#include "Config.h"
#include "Log.h"
int main(int argc, char* argv[]) {

    Config& config = Config::getInstance();
    Logger::getInstance().setLogLevel(config.getLogLevel());
    LOG_INFO("Server Host : {}", config.getHost());
    LOG_INFO("Server Port : {}", config.getPort());
    LOG_INFO("Database Name: = {}", config.getDbName());
    LOG_INFO("Log Level:  = {}", config.getLogLevel());
    while(true)
    {
        try {
            boost::asio::io_context io_context;
            tcp::endpoint endpoint(tcp::v4(), config.getPort());
            Server server(io_context, endpoint);
            std::cout << "Server Start ..." << std::endl;
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        Utils::printProcessStats(getpid());
        std::cout << "Server Restart ..." << std::endl;
    }


    return 0;
}
