#ifndef __SERVER_H__
#define __SERVER_H__

#include "game/manager.h"

#include "utils/connection_server.h"

#include <iostream>

namespace banggame {
    using bang_message_types = net::message_types<client_message, server_message, bang_header>;

    template<typename Derived>
    struct bang_server : net::connection_server<Derived, bang_message_types> {
        using base = net::connection_server<Derived, bang_message_types>;
        using base::connection_server;
        
        game_manager m_mgr;

        std::jthread m_game_thread;

        bool start(uint16_t port) {
            if (!base::start(port)) return false;

            m_mgr.set_send_message_function([&](int client_id, server_message message) {
                base::push_message(client_id, std::move(message));
            });

            m_game_thread = std::jthread(std::bind_front(&game_manager::start, &m_mgr));

            return true;
        }

        void on_receive_message(int client_id, client_message &&msg) {
            m_mgr.on_receive_message(client_id, std::move(msg));
        }

        void on_disconnect(int client_id) {
            m_mgr.client_disconnected(client_id);
        }

        bool client_validated(int client_id) const {
            return m_mgr.client_validated(client_id);
        }
    };
}

#endif