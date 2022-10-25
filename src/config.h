#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "image_serial.h"

#include "widgets/profile_pic.h"

#include "utils/resource.h"

#include "game/card_enums.h"

DEFINE_STRUCT(config,
    (std::vector<std::string>, recent_servers)
    (std::string, user_name)
    (std::string, profile_image)
    (sdl::surface, profile_image_data)
    (std::string, lobby_name)
    (banggame::game_options, options)
    (bool, allow_unofficial_expansions)
    (uint16_t, server_port),
    
    void load();
    void save();
)

#endif