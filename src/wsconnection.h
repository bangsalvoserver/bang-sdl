#ifndef __WSCONNECTION_H__
#define __WSCONNECTION_H__

#include <asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

namespace net {

    class wsconnection {
    public:
        using client_type = websocketpp::client<websocketpp::config::asio_client>;
        using client_handle = websocketpp::connection_hdl;

    private:
        client_type m_client;
        client_type::connection_weak_ptr m_con;

        std::string m_address;
    
    protected:
        virtual void on_open() = 0;
        virtual void on_close() = 0;
        virtual void on_message(const std::string &message) = 0;

    public:
        wsconnection(asio::io_context &ctx);

        virtual ~wsconnection() = default;
            
        void connect(const std::string &url);
        
        void disconnect();

        const std::string &address_string() const {
            return m_address;
        }

        void push_message(const std::string &message);
    };
}

#endif