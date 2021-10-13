#include <SDL2/SDL.h>
#include <iostream>
#include <map>

#include "manager.h"
#include "common/message_header.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    sdlnet::socket_set set(banggame::server_max_clients);
    sdlnet::tcp_server_socket server(banggame::server_port);
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
                        auto str = recv_message_string(it->second, header.length);
                        switch (header.type) {
                        case message_header::json:
                            mgr.parse_message(it->first, str);
                            break;
                        default:
                            break;
                        }
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
                    std::stringstream ss;
                    ss << msg.value;
                    std::string str = ss.str();

                    send_message_header(it->second, message_header{message_header::json, (uint32_t)str.size()});
                    send_message_string(it->second, str);
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