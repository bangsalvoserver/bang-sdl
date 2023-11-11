#include "wsconnection.h"

namespace net {

template<typename T, typename U>
bool check_weak_ptr(const std::weak_ptr<T> &lhs, const std::weak_ptr<U> &rhs) {
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

wsconnection::wsconnection(asio::io_context &ctx) {
    m_client.init_asio(&ctx);
    m_client.set_access_channels(websocketpp::log::alevel::none);
    m_client.set_error_channels(websocketpp::log::alevel::none);
}
        
void wsconnection::connect(const std::string &url) {
    m_client.set_open_handler([this](client_handle hdl) {
        if (check_weak_ptr(hdl, m_con)) {
            on_open();
        }
    });

    auto client_disconnect_handler = [this](client_handle hdl) {
        if (check_weak_ptr(hdl, m_con)) {
            on_close();
            m_con.reset();
        }
    };

    m_client.set_close_handler(client_disconnect_handler);
    m_client.set_fail_handler(client_disconnect_handler);

    m_client.set_message_handler([this](client_handle hdl, client_type::message_ptr msg) {
        if (check_weak_ptr(hdl, m_con)) {
            on_message(msg->get_payload());
        }
    });

    std::error_code ec;
    m_address = url;
    auto con = m_client.get_connection(std::string("ws://") + url, ec);
    if (!ec) {
        m_client.connect(con);
        m_con = con;
    }
}

void wsconnection::disconnect() {
    if (auto con = m_con.lock()) {
        switch (con->get_state()) {
        case websocketpp::session::state::connecting:
            con->terminate(make_error_code(websocketpp::error::operation_canceled));
            break;
        case websocketpp::session::state::open:
            con->close(0, "");
            break;
        }
    }
}

void wsconnection::push_message(const std::string &message) {
    std::error_code ec;
    m_client.send(m_con, message, websocketpp::frame::opcode::text, ec);
}

}