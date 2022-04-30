#include <iostream>
#include <charconv>

#include "game/net_options.h"

#include "server.h"

struct console_bang_server : banggame::bang_server<console_bang_server> {
    using banggame::bang_server<console_bang_server>::bang_server;

    void print_message(const std::string &message) {
        std::cout << message << '\n';
    }

    void print_error(const std::string &message) {
        std::cerr << message << '\n';
    }
};

int main(int argc, char **argv) {
    boost::asio::io_context ctx;

    console_bang_server server(ctx);

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
        ctx_thread.join();
    }

    return 0;
}