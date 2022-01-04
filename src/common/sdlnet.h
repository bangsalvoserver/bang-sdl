#ifndef __SDLNET_H__
#define __SDLNET_H__

#include <SDL2/SDL_net.h>
#include <stdexcept>
#include <sstream>

namespace sdlnet {

    constexpr size_t buffer_size = 1024;

    struct net_error : std::runtime_error {
        using std::runtime_error::runtime_error;
        
        net_error() : std::runtime_error(SDLNet_GetError()) {}
    };

    struct socket_disconnected : std::runtime_error {
        socket_disconnected() : std::runtime_error("Socket disconnected") {}
    };

    struct initializer {
        initializer() {
            if (SDLNet_Init()==-1) {
                throw net_error();
            }
        }

        ~initializer() {
            SDLNet_Quit();
        }

        initializer(const initializer &other) = delete;
        initializer(initializer &&other) = delete;
        initializer &operator = (const initializer &other) = delete;
        initializer &operator = (initializer &&other) = delete;
    };

    struct ip_address : IPaddress {
        ip_address() = default;

        ip_address(IPaddress addr) : IPaddress(addr) {}

        ip_address(uint32_t host, uint16_t port) {
            SDLNet_Write32(host, &this->host);
            SDLNet_Write16(port, &this->port);
        }

        ip_address(const std::string &host, uint16_t port) {
            if (SDLNet_ResolveHost(this, host.c_str(), port) == -1) {
                throw net_error();
            }
        }

        std::string host_string() const {
            return SDLNet_ResolveIP(this);
        }

        std::string ip_string() const {
            std::stringstream ss;
            uint32_t host_value = SDLNet_Read32(&this->host);
            uint16_t host_port = SDLNet_Read16(&this->port);
            ss  << ((host_value & 0xff000000) >> (8 * 3)) << '.'
                << ((host_value & 0x00ff0000) >> (8 * 2)) << '.'
                << ((host_value & 0x0000ff00) >> (8 * 1)) << '.'
                << ((host_value & 0x000000ff) >> (8 * 0)) << ':'
                << host_port;
            return ss.str();
        }

        bool operator == (const ip_address &addr) const {
            return this->host == addr.host && this->port == addr.port;
        }

        auto operator <=> (const ip_address &addr) const {
            return this->host == addr.host
                ? this->port <=> addr.port
                : this->host <=> addr.host;
        }
    };

    struct tcp_socket {
        TCPsocket sock = nullptr;

        tcp_socket() = default;

        tcp_socket(const tcp_socket &other) = delete;
        tcp_socket(tcp_socket &&other) {
            std::swap(sock, other.sock);
        }

        tcp_socket &operator = (const tcp_socket &other) = delete;
        tcp_socket &operator = (tcp_socket &&other) {
            std::swap(sock, other.sock);
            return *this;
        }

        tcp_socket(TCPsocket sock) : sock(sock) {
            if (!sock) throw net_error();
        }

        explicit tcp_socket(ip_address addr) {
            open(addr);
        }

        ~tcp_socket() {
            close();
        }

        bool isopen() {
            return sock != nullptr;
        }

        void open(ip_address addr) {
            if (sock) {
                close();
            }
            sock = SDLNet_TCP_Open(&addr);
            if (!sock) {
                throw net_error();
            }
        }

        void close() {
            if (sock) {
                SDLNet_TCP_Close(sock);
                sock = nullptr;
            }
        }

        int send(const void *data, int len) const {
            int nbytes = SDLNet_TCP_Send(sock, data, len);
            if (nbytes <= 0) {
                throw socket_disconnected();
            }
            return nbytes;
        }

        int recv(void *data, int maxlen) const {
            int nbytes = SDLNet_TCP_Recv(sock, data, maxlen);
            if (nbytes <= 0) {
                throw socket_disconnected();
            }
            return nbytes;
        }
    };

    struct tcp_peer_socket : tcp_socket {
        ip_address addr;

        tcp_peer_socket(TCPsocket sock)
            : tcp_socket(sock)
            , addr(*SDLNet_TCP_GetPeerAddress(sock)) {}
    };

    struct socket_set {
        static constexpr int default_maxsockets = 16;
        
        SDLNet_SocketSet set;

        socket_set(const socket_set &other) = delete;
        socket_set(socket_set &&other) {
            std::swap(set, other.set);
        }

        socket_set &operator = (const socket_set &other) = delete;
        socket_set &operator = (socket_set &&other) {
            std::swap(set, other.set);
            return *this;
        }

        socket_set(int maxsockets = default_maxsockets) {
            set = SDLNet_AllocSocketSet(maxsockets);
        }

        ~socket_set() {
            SDLNet_FreeSocketSet(set);
        }

        void add(const tcp_socket &sock) {
            if (SDLNet_TCP_AddSocket(set, sock.sock) == -1) {
                throw net_error();
            } 
        }

        void erase(const tcp_socket &sock) {
            if (SDLNet_TCP_DelSocket(set, sock.sock) == -1) {
                throw net_error();
            }
        }

        int check(uint32_t timeout) {
            int num = SDLNet_CheckSockets(set, timeout);
            if (num == -1) {
                throw net_error();
            }
            return num;
        }

        bool ready(const tcp_socket &sock) {
            return SDLNet_SocketReady(sock.sock);
        }
    };

    struct tcp_server_socket : tcp_socket {
        explicit tcp_server_socket(uint16_t port) : tcp_socket(ip_address(INADDR_ANY, port)) {}

        tcp_peer_socket accept() {
            return SDLNet_TCP_Accept(sock);
        }
    };

}

#endif