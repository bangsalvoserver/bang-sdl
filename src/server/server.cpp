#include "server.h"

#include "common/message_header.h"
#include "common/binary_serial.h"

#include "manager.h"

using namespace std::string_literals;
using namespace std::placeholders;

bang_server::bang_server()
    : m_sockset(banggame::server_max_clients) {}

bool bang_server::start() {
    try {
        m_socket.listen(banggame::server_port);
    } catch (const sdlnet::net_error &error) {
        print_error(error.what());
        return false;
    }

    print_message("Server listening on port "s + std::to_string(banggame::server_port));

    m_sockset.add(m_socket);

    m_thread = std::jthread([this](std::stop_token token) {
        game_manager mgr;
        mgr.set_error_callback(std::bind(&bang_server::print_error, this, _1));
        while(!token.stop_requested()) {
            if (m_sockset.check(0)) {
                for (auto it = m_clients.begin(); it != m_clients.end();) {
                    try {
                        if (m_sockset.ready(it->second)) {
                            mgr.parse_message(it->first, recv_message_bytes(it->second));
                        }
                        ++it;
                    } catch (sdlnet::socket_disconnected) {
                        mgr.client_disconnected(it->first);
                        print_message(it->first.ip_string() + " Disconnected"s);
                        m_sockset.erase(it->second);
                        it = m_clients.erase(it);
                    } catch (const std::exception &e) {
                        mgr.client_disconnected(it->first);
                        print_error(it->first.ip_string() + ": "s + e.what());
                        m_sockset.erase(it->second);
                        it = m_clients.erase(it);
                    }
                }
                if (m_sockset.ready(m_socket)) {
                    auto peer = m_socket.accept();
                    m_sockset.add(peer);
                    print_message(peer.addr.ip_string() + " Connected"s);
                    m_clients.emplace(peer.addr, std::move(peer));
                }
            }
            mgr.tick();
            while (mgr.pending_messages()) {
                auto msg = mgr.pop_message();
                
                auto it = m_clients.find(msg.addr);
                if (it != m_clients.end()) {
                    try {
                        send_message_bytes(it->second, binary::serialize(msg.value));
                    } catch (sdlnet::socket_disconnected) {
                        mgr.client_disconnected(it->first);
                        print_message(it->first.ip_string() + " Disconnected"s);
                        m_sockset.erase(it->second);
                        m_clients.erase(it);
                    }
                }
            }
            SDL_Delay(1000 / banggame::fps);
        }
    });

    return true;
}