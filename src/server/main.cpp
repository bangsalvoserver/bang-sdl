#include <SDL2/SDL.h>
#include <iostream>

#include "server.h"
#include "common/net_options.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    bang_server server(SDL_GetBasePath());
    
    server.set_message_callback([](const std::string &message) {
        std::cout << message << '\n';
    });
    
    server.set_error_callback([](const std::string &message) {
        std::cerr << message << '\n';
    });

    if (server.start()) {
        server.join();
    }

    return 0;
}