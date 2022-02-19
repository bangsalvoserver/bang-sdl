#include <iostream>

#include "server.h"
#include "net_options.h"

int main(int argc, char **argv) {
    boost::asio::io_context ctx;

    bang_server server(ctx, std::filesystem::path(argv[0]).parent_path());
    
    server.set_message_callback([](const std::string &message) {
        std::cout << message << '\n';
    });
    
    server.set_error_callback([](const std::string &message) {
        std::cerr << message << '\n';
    });

    if (server.start()) {
        std::thread ctx_thread([&]{ ctx.run(); });

        server.join();
        ctx_thread.join();
    }

    return 0;
}