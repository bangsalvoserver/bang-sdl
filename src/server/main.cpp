#include <SDL2/SDL.h>
#include <iostream>
#include <map>

#include "manager.h"
#include "common/message_header.h"
#include "common/binary_serial.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    sdlnet::socket_set set(banggame::server_max_clients);
    sdlnet::tcp_server_socket server(banggame::server_port);

    std::cout << "Server listening on port " << banggame::server_port << '\n';

    set.add(server);

    std::map<sdlnet::ip_address, sdlnet::tcp_peer_socket> clients;

    game_manager mgr;

    bool quit = false;
    while(!quit) {
        if (set.check(0)) {
            for (auto it = clients.begin(); it != clients.end();) {
                try {
                    if (set.ready(it->second)) {
                        auto header = recv_message_header(it->second);
                        mgr.parse_message(it->first, recv_message_bytes(it->second, header.length));
                    }
                    ++it;
                } catch (sdlnet::socket_disconnected) {
                    mgr.client_disconnected(it->first);
                    std::cout << it->first.ip_string() << " Disconnected\n";
                    set.erase(it->second);
                    it = clients.erase(it);
                }
            }
            if (set.ready(server)) {
                auto peer = server.accept();
                set.add(peer);
                std::cout << peer.addr.ip_string() << " Connected\n";
                clients.emplace(peer.addr, std::move(peer));
            }
        }
        mgr.tick();
        while (mgr.pending_messages()) {
            auto msg = mgr.pop_message();
            
            auto it = clients.find(msg.addr);
            if (it != clients.end()) {
                try {
                    send_message_header(it->second, message_header{(uint32_t)msg.value.size()});
                    send_message_bytes(it->second, msg.value);
                } catch (sdlnet::socket_disconnected) {
                    mgr.client_disconnected(it->first);
                    std::cout << it->first.ip_string() << " Disconnected\n";
                    set.erase(it->second);
                    clients.erase(it);
                }
            }
        }
        SDL_Delay(1000 / banggame::fps);
    }

    return 0;
}