#include <SDL2/SDL.h>
#include <iostream>
#include <map>

#include "manager.h"

int main(int argc, char **argv) {
    sdlnet::initializer init;

    sdlnet::socket_set set(banggame::server_max_clients);
    sdlnet::tcp_server_socket server(banggame::server_port);
    set.add(server);

    std::map<sdlnet::ip_address, sdlnet::tcp_peer_socket> clients;

    game_manager mgr;

    bool quit = false;
    while(!quit) {
        if (set.check(banggame::socket_set_timeout)) {
            for (auto it = clients.begin(); it != clients.end();) {
                try {
                    if (set.ready(it->second)) {
                        std::string str = it->second.recv_string();
                        mgr.parse_message(it->first, str);
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
            while (mgr.pending_messages()) {
                auto msg = mgr.pop_message();
                auto it = clients.find(msg.addr);
                if (it != clients.end()) {
                    Json::Value json_msg = Json::objectValue;
                    json_msg["type"] = std::string(enums::to_string(msg.type));
                    if (!msg.value.isNull()) {
                        json_msg["value"] = msg.value;
                    }

                    std::stringstream ss;
                    ss << json_msg;
                    it->second.send_string(ss.str());
                }
            }
        }
    }

    return 0;
}