#include "wsconnection.h"

namespace net {
        
void wsconnection::connect(const std::string &url) {
    auto init_client = [&]<typename Config>(std::in_place_type_t<Config>) -> decltype(auto) {
        auto &client = m_client.emplace<client_and_connection<Config>>().client;
        client.init_asio(&m_ctx);

        client.set_access_channels(websocketpp::log::alevel::none);
        client.set_error_channels(websocketpp::log::alevel::none);
        
        client.set_open_handler([this](client_handle hdl) {
            on_open();
        });
        client.set_close_handler([this](client_handle hdl){
            on_close();
            m_client = std::monostate{};
        });
        client.set_fail_handler([this](client_handle hdl){
            on_close();
            m_client = std::monostate{};
        });
        client.set_message_handler([this](client_handle hdl, websocketpp::client<Config>::message_ptr msg) {
            on_message(msg->get_payload());
        });

        return client;
    };

    auto uri = std::make_shared<websocketpp::uri>(url);
    if (uri->get_secure()) {
        init_client(std::in_place_type<websocketpp::config::asio_tls_client>)
            .set_tls_init_handler([](client_handle hdl) {
                return std::make_shared<asio::ssl::context>(asio::ssl::context::tls_client);
            });
    } else {
        init_client(std::in_place_type<websocketpp::config::asio_client>);
    }

    bool result = std::visit(overloaded{
        [](std::monostate) { return false; },
        [&]<typename Config>(client_and_connection<Config> &client) {
            std::error_code ec;
            auto con = client.client.get_connection(uri, ec);
            if (!ec) {
                client.client.connect(con);
                client.connection = con;
                return true;
            }
            return false;
        }
    }, m_client);

    if (!result) {
        m_client = std::monostate{};
    }
}

void wsconnection::disconnect() {
    std::visit(overloaded{
        [](std::monostate) {},
        []<typename Config>(client_and_connection<Config> &client) {
            if (auto ptr = client.connection.lock()) {
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
    }, m_client);
}

void wsconnection::poll() {
    m_ctx.poll();
}

void wsconnection::push_message(const std::string &message) {
    std::error_code ec;
    std::visit(overloaded{
        [](std::monostate) {},
        [&]<typename Config>(client_and_connection<Config> &client) {
            client.client.send(client.connection, message, websocketpp::frame::opcode::text, ec);
        },
    }, m_client);
}

}