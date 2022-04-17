#ifndef __NET_OPTIONS_H__
#define __NET_OPTIONS_H__

#include "utils/reflector.h"

namespace banggame {
    constexpr uint16_t default_server_port = 47654;
    constexpr int server_max_clients = 100;
    constexpr int lobby_max_players = 8;
    constexpr int fps = 60;

    struct bang_header {
        static constexpr uint32_t magic_number = 0x42414e47;
        static constexpr uint16_t bang_version = 0xb003;

        REFLECTABLE(
            (uint32_t) magic,
            (uint16_t) version,
            (uint16_t) length /* max packet size 64 kB */
        )

        bang_header()
            : magic(magic_number)
            , version(bang_version) {}
        
        bool validate() const {
            return magic == magic_number
                && version == bang_version;
        }
    };

}

#endif