#include <iostream>
#include <charconv>

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

    uint16_t port = banggame::default_server_port;
    if (argc > 1) {
        auto [ptr, ec] = std::from_chars(argv[1], argv[1] + strlen(argv[1]), port);
        if (ec != std::errc{}) {
            std::cerr << "Port must be a number\0";
            return 1;
        }
    }

    if (server.start(port)) {
        std::thread ctx_thread([&]{ ctx.run(); });

        server.join();
        ctx_thread.join();
    }

    return 0;
}