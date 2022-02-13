#include <SDL2/SDL.h>
#include <iostream>

#include "server.h"
#include "common/options.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    banggame::globals::base_path = SDL_GetBasePath();

    bang_server server;
    
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