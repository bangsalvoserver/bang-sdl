#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "image_serial.h"

#include "widgets/profile_pic.h"

#include "utils/resource.h"

#include "cards/card_enums.h"

#ifndef OFFICIAL_BANG_SERVER
    #define OFFICIAL_BANG_SERVER
#endif

DEFINE_STRUCT(config,
    (std::vector<std::string>, recent_servers, OFFICIAL_BANG_SERVER)
    (std::string, user_name)
    (std::string, profile_image)
    (sdl::surface, profile_image_data)
    (std::string, lobby_name)
    (banggame::game_options, options)
    (bool, allow_unofficial_expansions)
    (bool, bypass_prompt)
    (float, sound_volume, .5f)
    (uint16_t, server_port)
    (bool, server_enable_cheats)
    (bool, server_verbose),
    
    void load();
    void save();
)

#endif