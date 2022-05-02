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
        using connection_type = typename base::connection_type;
        using connection_handle = typename base::connection_handle;
        
        game_manager m_mgr;

        std::jthread m_game_thread;

        bool start(uint16_t port) {
            if (!base::start(port)) return false;

            m_mgr.set_send_message_function([&](client_handle client, server_message message) {
                if (auto ptr = std::static_pointer_cast<connection_type>(client.lock())) {
                    ptr->push_message(std::move(message));
                }
            });

            m_game_thread = std::jthread(std::bind_front(&game_manager::start, &m_mgr));

            return true;
        }

        void on_receive_message(connection_handle client, client_message &&msg) {
            m_mgr.on_receive_message(client, std::move(msg));
        }

        void on_disconnect(connection_handle client) {
            m_mgr.client_disconnected(client);
        }

        bool client_validated(connection_handle client) const {
            return m_mgr.client_validated(client);
        }
    };
}

#endif