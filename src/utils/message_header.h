#ifndef __MESSAGE_HEADER_H__
#define __MESSAGE_HEADER_H__

#include "sdlnet.h"

constexpr uint32_t bang_magic = 0x42414e47;
constexpr size_t buffer_size = 1024;

struct magic_number_mismatch : std::runtime_error  {
    magic_number_mismatch() : std::runtime_error("Magic number mismatch") {}
};

void send_message_bytes(sdlnet::tcp_socket &sock, const std::vector<std::byte> &bytes) {
    std::byte buf[sizeof(uint32_t) * 2];
    *reinterpret_cast<uint32_t*>(buf) = sdlnet::host_to_net32(bang_magic);
    *reinterpret_cast<uint32_t*>(buf + sizeof(uint32_t)) = sdlnet::host_to_net32(bytes.size());
    sock.send(buf, sizeof(buf));

    const std::byte *pos = bytes.data();
    while (pos != bytes.data() + bytes.size()) {
        pos += sock.send(pos, std::min(buffer_size, (size_t)(bytes.data() + bytes.size() - pos)));
    }
}

std::vector<std::byte> recv_message_bytes(sdlnet::tcp_socket &sock) {
    std::byte buf[sizeof(uint32_t) * 2];
    sock.recv(buf, sizeof(buf));

    if (sdlnet::net_to_host32(*reinterpret_cast<uint32_t*>(buf)) != bang_magic) {
        throw magic_number_mismatch();
    }

    std::vector<std::byte> ret(sdlnet::net_to_host32(*reinterpret_cast<uint32_t*>(buf + sizeof(uint32_t))));
    std::byte *pos = ret.data();
    while (pos != ret.data() + ret.size()) {
        pos += sock.recv(pos, std::min(buffer_size, (size_t)(ret.data() + ret.size() - pos)));
    }
    return ret;
}

#endif