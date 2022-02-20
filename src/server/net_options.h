#ifndef __NET_OPTIONS_H__
#define __NET_OPTIONS_H__

#include "utils/reflector.h"

namespace banggame {
    constexpr uint16_t server_port = 27015;
    constexpr int server_max_clients = 100;
    constexpr int lobby_max_players = 8;
    constexpr int fps = 60;

    struct bang_header {
        static constexpr uint32_t magic_number = 0x42414e47;
        static constexpr uint32_t max_length = 0x40000; // max packet 256 kiB

        REFLECTABLE(
            (uint32_t) magic,
            (uint32_t) length
        )

        bang_header() : magic(magic_number) {}
        
        bool validate() const {
            return magic == magic_number
                && length <= max_length;
        }
    };

}

#endif