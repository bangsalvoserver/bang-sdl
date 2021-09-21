#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <cstdint>

namespace banggame {
    constexpr uint16_t server_port = 12345;
    constexpr int server_max_clients = 100;
    constexpr uint32_t socket_set_timeout = 1000;
}

#endif