#ifndef __MESSAGE_HEADER_H__
#define __MESSAGE_HEADER_H__

#include "utils/sdlnet.h"

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

void recv_message_header(sdlnet::tcp_socket &sock, message_header &header) {
    std::byte buf[sizeof(message_header)];
    sock.recv(&buf, sizeof(buf));
    header.type = static_cast<message_header::message_type>(SDLNet_Read32(buf));
    header.length = SDLNet_Read32(buf + 4);
}

#endif