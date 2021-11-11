#ifndef __MESSAGE_HEADER_H__
#define __MESSAGE_HEADER_H__

#include "common/sdlnet.h"

struct message_header {
    enum message_type : uint32_t {
        none,
        json,
        binary
    };

    message_type type;
    uint32_t length;
};

void send_message_header(sdlnet::tcp_socket &sock, const message_header &header) {
    std::byte buf[sizeof(message_header)];
    SDLNet_Write32(static_cast<uint32_t>(header.type), buf);
    SDLNet_Write32(header.length, buf + 4);
    sock.send(&buf, sizeof(buf));
}

message_header recv_message_header(sdlnet::tcp_socket &sock) {
    message_header header;
    std::byte buf[sizeof(message_header)];
    sock.recv(&buf, sizeof(buf));
    header.type = static_cast<message_header::message_type>(SDLNet_Read32(buf));
    header.length = SDLNet_Read32(buf + 4);
    return header;
}

constexpr size_t buffer_size = 1024;

void send_message_string(sdlnet::tcp_socket &sock, const std::string &str) {
    const char *pos = str.data();
    while (pos != str.data() + str.size()) {
        pos += sock.send(pos, std::min(buffer_size, (size_t)(str.data() + str.size() - pos)));
    }
}

std::string recv_message_string(sdlnet::tcp_socket &sock, int length) {
    std::string ret(length, '\0');
    char *pos = ret.data();
    while (pos != ret.data() + ret.size()) {
        pos += sock.recv(pos, std::min(buffer_size, (size_t)(ret.data() + ret.size() - pos)));
    }
    return ret;
}

#endif