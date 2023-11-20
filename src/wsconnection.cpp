#include "wsconnection.h"

namespace net {

wsconnection::wsconnection(asio::io_context &ctx) {
    auto init_client = [&]<typename Config>(websocketpp::client<Config> &client) {
        client.init_asio(&ctx);
        client.set_access_channels(websocketpp::log::alevel::none);
        client.set_error_channels(websocketpp::log::alevel::none);
        client.set_open_handler([this](client_handle hdl) {
            on_open();
        });
        client.set_close_handler([this](client_handle hdl){
            on_close();
            m_con = std::monostate{};
        });
        client.set_fail_handler([this](client_handle hdl){
            on_close();
            m_con = std::monostate{};
        });
        client.set_message_handler([this](client_handle hdl, websocketpp::client<Config>::message_ptr msg) {
            on_message(msg->get_payload());
        });
    };

    init_client(m_client);
    init_client(m_client_tls);

    m_client_tls.set_tls_init_handler([](client_handle hdl) {
        return std::make_shared<asio::ssl::context>(asio::ssl::context::tls_client);
    });
}
        
void wsconnection::connect(const std::string &url) {
    std::error_code ec;

    auto uri = std::make_shared<websocketpp::uri>(url);
    if (uri->get_secure()) {
        auto con = m_client_tls.get_connection(uri, ec);
        if (!ec) {
            m_client_tls.connect(con);
            m_con = con;
            m_address = url;
        }
    } else {
        auto con = m_client.get_connection(uri, ec);
        if (!ec) {
            m_client.connect(con);
            m_con = con;
            m_address = url;
        }
    }
}

void wsconnection::disconnect() {
    std::visit([](auto &con) {
        if constexpr (requires { con.lock(); }) {
            if (auto ptr = con.lock()) {
                switch (ptr->get_state()) {
                case websocketpp::session::state::connecting:
                    ptr->terminate(make_error_code(websocketpp::error::operation_canceled));
                    break;
                case websocketpp::session::state::open:
                    ptr->close(0, "DISCONNECT");
                    break;
                }
            }
        }
    }, m_con);
}

void wsconnection::push_message(const std::string &message) {
    std::error_code ec;
    std::visit(overloaded{
        [](std::monostate) {},
        [&](client_type::connection_weak_ptr con) {
            m_client.send(con, message, websocketpp::frame::opcode::text, ec);
        },
        [&](client_type_tls::connection_weak_ptr con) {
            m_client_tls.send(con, message, websocketpp::frame::opcode::text, ec);
        }
    }, m_con);
}

}