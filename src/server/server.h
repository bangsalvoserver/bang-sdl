#ifndef __SERVER_H__
#define __SERVER_H__

#include <filesystem>
#include <functional>
#include <thread>
#include <map>

#include "utils/connection.h"

#include "server/net_enums.h"
#include "server/net_options.h"

#include "ansicvt.h"

using message_callback_t = std::function<void(const std::string &)>;

struct game_manager;

class bang_server {
public:
    bang_server(boost::asio::io_context &ctx, const std::filesystem::path &base_path);

    void set_message_callback(message_callback_t &&fun) {
        m_message_callback = std::move(fun);
    }

    void set_error_callback(message_callback_t &&fun) {
        m_error_callback = std::move(fun);
    }

    bool start(uint16_t port);

    void join() {
        if (m_game_thread.joinable()) {
            m_game_thread.join();
        }
    }
    
    void stop();

private:
    boost::asio::io_context &m_ctx;
    boost::asio::ip::tcp::acceptor m_acceptor;

    std::unique_ptr<game_manager, void (*)(game_manager *)> m_mgr;

    void start_accepting();

    using connection_type = net::connection<net::message_types<client_message, server_message, banggame::bang_header>>;
    std::map<int, connection_type::pointer> m_clients;

    int m_client_id_counter = 0;

    std::filesystem::path m_base_path;

    std::jthread m_game_thread;

    message_callback_t m_message_callback;
    message_callback_t m_error_callback;

    void print_message(const std::string &msg) {
        if (m_message_callback) m_message_callback(msg);
    }

    void print_error(const std::string &msg) {
        if (m_error_callback) m_error_callback(msg);
    }
};

#endif