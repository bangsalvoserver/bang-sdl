#include <SDL2/SDL.h>
#include <iostream>
#include <thread>

#include "utils/sdlnet.h"
#include "common/options.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    sdlnet::tcp_socket sock(sdlnet::ip_address("localhost", banggame::server_port));
    std::thread recv_thread([&]{
        sdlnet::socket_set set(10);
        set.add(sock);

        while(true) {
            try {
                if (set.check(1000) && set.ready(sock)) {
                    std::cout << sock.recv_string() << '\n';
                }
            } catch (sdlnet::socket_disconnected) {
                break;
            } catch (sdlnet::sdlnet_error) {
                break;
            }
        }
    });

    std::string line;
    while(true) {
        std::getline(std::cin, line);
        if (line == "quit") break;
        sock.send_string(line);
    }

    sock.close();
    recv_thread.join();

    return 0;
}