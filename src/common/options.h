#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <cstdint>
#include <string>

namespace banggame {
    constexpr uint16_t server_port = 27015;
    constexpr int server_max_clients = 100;
    constexpr int lobby_max_players = 8;
    constexpr int fps = 60;

    namespace globals {

        extern std::string base_path;

    }
}

#endif