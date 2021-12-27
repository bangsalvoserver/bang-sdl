#ifndef __MESSAGE_HEADER_H__
#define __MESSAGE_HEADER_H__

#include "common/sdlnet.h"

struct message_header {
    uint32_t length;
};

void send_message_header(sdlnet::tcp_socket &sock, const message_header &header) {
    std::byte buf[sizeof(message_header)];
    SDLNet_Write32(header.length, buf);
    sock.send(&buf, sizeof(buf));
}

message_header recv_message_header(sdlnet::tcp_socket &sock) {
    message_header header;
    std::byte buf[sizeof(message_header)];
    sock.recv(&buf, sizeof(buf));
    header.length = SDLNet_Read32(buf);
    return header;
}

constexpr size_t buffer_size = 1024;

void send_message_bytes(sdlnet::tcp_socket &sock, const std::vector<std::byte> &str) {
    const std::byte *pos = str.data();
    while (pos != str.data() + str.size()) {
        pos += sock.send(pos, std::min(buffer_size, (size_t)(str.data() + str.size() - pos)));
    }
}

std::vector<std::byte> recv_message_bytes(sdlnet::tcp_socket &sock, int length) {
    std::vector<std::byte> ret(length);
    std::byte *pos = ret.data();
    while (pos != ret.data() + ret.size()) {
        pos += sock.recv(pos, std::min(buffer_size, (size_t)(ret.data() + ret.size() - pos)));
    }
    return ret;
}

#endif