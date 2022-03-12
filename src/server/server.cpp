#include "server.h"

#include "utils/binary_serial.h"

#include "manager.h"

#include <fmt/core.h>

bang_server::bang_server(boost::asio::io_context &ctx, const std::filesystem::path &base_path)
    : m_ctx(ctx)
    , m_acceptor(ctx)
    , m_mgr(nullptr, [](game_manager *value) { delete value; })
    , m_base_path(base_path) {}

void bang_server::start_accepting() {
    m_acceptor.async_accept(
        [this](const boost::system::error_code &ec, boost::asio::ip::tcp::socket peer) {
            if (!ec) {
                if (m_clients.size() < banggame::server_max_clients) {
                    auto client = connection_type::make(m_ctx, std::move(peer));
                    client->start();
                    
                    print_message(fmt::format("{} connected", client->address_string()));

                    auto it = m_clients.emplace(++m_client_id_counter, std::move(client)).first;

                    using timer_type = boost::asio::basic_waitable_timer<std::chrono::system_clock>;
                    auto timer = new timer_type(m_ctx);

                    timer->expires_after(net::timeout);
                    timer->async_wait(
                        [this,
                        timer = std::unique_ptr<timer_type>(timer),
                        client_id = it->first, ptr = std::weak_ptr(it->second)](const boost::system::error_code &ec) {
                            if (!ec) {
                                if (auto client = ptr.lock()) {
                                    if (!m_mgr->client_validated(client_id)) {
                                        client->disconnect(net::connection_error::timeout_expired);
                                    }
                                }
                            }
                        });
                } else {
                    peer.close();
                }
            }
            if (ec != boost::asio::error::operation_aborted) {
                start_accepting();
            }
        });
}

bool bang_server::start(uint16_t port) {
    try {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        m_acceptor.open(endpoint.protocol());
        m_acceptor.bind(endpoint);
        m_acceptor.listen();
    } catch (const boost::system::system_error &error) {
        print_error(ansi_to_utf8(error.code().message()));
        return false;
    }

    print_message(fmt::format("Server listening on port {}", port));

    start_accepting();

    m_game_thread = std::jthread([this](std::stop_token token) {
        m_mgr.reset(new game_manager(m_base_path));

        using frames = std::chrono::duration<int64_t, std::ratio<1, banggame::fps>>;
        auto next_frame = std::chrono::steady_clock::now() + frames{0};

        while (!token.stop_requested()) {
            next_frame += frames{1};

            for (auto it = m_clients.begin(); it != m_clients.end();) {
                switch (it->second->state()) {
                case net::connection_state::error:
                    print_message(fmt::format("{} disconnected ({})", it->second->address_string(), ansi_to_utf8(it->second->error_message())));
                    m_mgr->client_disconnected(it->first);
                    it = m_clients.erase(it);
                    break;
                case net::connection_state::disconnected:
                    print_message(fmt::format("{} disconnected", it->second->address_string()));
                    m_mgr->client_disconnected(it->first);
                    it = m_clients.erase(it);
                    break;
                case net::connection_state::connected:
                    while (auto msg = it->second->pop_message()) {
                        try {
                            m_mgr->handle_message(it->first, *msg);
                        } catch (game_manager::invalid_message) {
                            it->second->disconnect(net::connection_error::invalid_message);
                        } catch (const std::exception &error) {
                            print_error(fmt::format("Error: {}", error.what()));
                        }
                    }
                    [[fallthrough]];
                default:
                    ++it;
                }
            }
            m_mgr->tick();
            while (m_mgr->pending_messages()) {
                auto msg = m_mgr->pop_message();
                
                auto it = m_clients.find(msg.client_id);
                if (it != m_clients.end()) {
                    it->second->push_message(std::move(msg.value));
                }
            }
            
            std::this_thread::sleep_until(next_frame);
        }

        m_mgr.reset();
        print_message("Server shut down");
    });

    return true;
}

void bang_server::stop() {
    m_game_thread.request_stop();
    m_acceptor.close();

    for (auto &[id, con] : m_clients) {
        con->disconnect();
    }
}