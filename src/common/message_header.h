#ifndef __MESSAGE_HEADER_H__
#define __MESSAGE_HEADER_H__

#include "common/sdlnet.h"

constexpr uint32_t bang_magic = 0x42414e47;
constexpr size_t buffer_size = 1024;

void send_message_bytes(sdlnet::tcp_socket &sock, const std::vector<std::byte> &bytes) {
    std::byte buf[sizeof(uint32_t) * 2];
    SDLNet_Write32(bang_magic, buf);
    SDLNet_Write32(static_cast<uint32_t>(bytes.size()), buf + sizeof(uint32_t));
    sock.send(buf, sizeof(buf));

    const std::byte *pos = bytes.data();
    while (pos != bytes.data() + bytes.size()) {
        pos += sock.send(pos, std::min(buffer_size, (size_t)(bytes.data() + bytes.size() - pos)));
    }
}

std::vector<std::byte> recv_message_bytes(sdlnet::tcp_socket &sock) {
    std::byte buf[sizeof(uint32_t) * 2];
    sock.recv(buf, sizeof(buf));
    if (SDLNet_Read32(buf) != bang_magic) {
        throw sdlnet::net_error("Magic number mismatch");
    }

    std::vector<std::byte> ret(SDLNet_Read32(buf + sizeof(uint32_t)));
    std::byte *pos = ret.data();
    while (pos != ret.data() + ret.size()) {
        pos += sock.recv(pos, std::min(buffer_size, (size_t)(ret.data() + ret.size() - pos)));
    }
    return ret;
}

#endif