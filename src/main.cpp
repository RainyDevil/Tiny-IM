#include <iostream>
#include "Server.h"
#include "Config.h"
#include "Log.h"
int main(int argc, char* argv[]) {

    Config& config = Config::getInstance();
    std::cout << "Port: " << config.getPort() << std::endl;
    std::cout << "Database Name: " << config.getDbName() << std::endl;
    std::cout << "Host: " << config.getHost() << std::endl;
    std::cout << "Log Level: " << config.getLogLevel() << std::endl;
    LOG_INFO("This is an info message with a number: {}", 42);
    LOG_DEBUG("Debug message: var1 = {}, var2 = {}", "test", 3.14);
    LOG_WARNING("Warning! Possible issue detected: {}", "low memory");
    LOG_ERROR("Error occurred: {}, {}", "file not found", 404);
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
