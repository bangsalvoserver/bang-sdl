#include "server.h"

#include "utils/binary_serial.h"

#include "manager.h"

using namespace std::string_literals;
using namespace std::placeholders;

bang_server::bang_server(boost::asio::io_context &ctx, const std::filesystem::path &base_path)
    : m_ctx(ctx)
    , m_acceptor(ctx)
    , m_base_path(base_path) {}

void bang_server::start_accepting() {
    m_acceptor.async_accept(
        [this](const boost::system::error_code &ec, boost::asio::ip::tcp::socket peer) {
            if (!ec) {
                auto client = connection_type::make(m_ctx, std::move(peer));
                client->start();
                
                print_message("Connection from "s + client->address_string());
                
                m_clients.emplace(++m_client_id_counter, std::move(client));
            }
            if (ec != boost::asio::error::operation_aborted) {
                start_accepting();
            }
        });
}

bool bang_server::start() {
    try {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), banggame::server_port);
        m_acceptor.open(endpoint.protocol());
        m_acceptor.bind(endpoint);
        m_acceptor.listen(banggame::server_max_clients);
    } catch (const boost::system::system_error &error) {
        print_error(error.code().message());
        return false;
    }

    print_message("Server listening on port "s + std::to_string(banggame::server_port));

    start_accepting();

    m_game_thread = std::jthread([this](std::stop_token token) {
        game_manager mgr{m_base_path};
        mgr.set_message_callback(std::bind(&bang_server::print_message, this, _1));
        mgr.set_error_callback(std::bind(&bang_server::print_error, this, _1));

        using frames = std::chrono::duration<int64_t, std::ratio<1, banggame::fps>>;
        auto next_frame = std::chrono::high_resolution_clock::now() + frames{0};

        while (!token.stop_requested()) {
            next_frame += frames{1};

            for (auto it = m_clients.begin(); it != m_clients.end();) {
                if (it->second->connected()) {
                    while (it->second->incoming_messages()) {
                        mgr.handle_message(it->first, it->second->pop_message());
                    }
                    ++it;
                } else {
                    mgr.client_disconnected(it->first);
                    it = m_clients.erase(it);
                }
            }
            mgr.tick();
            while (mgr.pending_messages()) {
                auto msg = mgr.pop_message();
                
                auto it = m_clients.find(msg.client_id);
                if (it != m_clients.end()) {
                    it->second->push_message(std::move(msg.value));
                }
            }
            
            std::this_thread::sleep_until(next_frame);
        }

        print_message("Server shut down");
    });

    return true;
}