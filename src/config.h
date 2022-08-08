#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "image_serial.h"

#include "widgets/profile_pic.h"

#include "utils/resource.h"

#include "game/card_enums.h"

struct config {
    REFLECTABLE(
        (std::vector<std::string>) recent_servers,
        (std::string) user_name,
        (std::string) profile_image,
        (sdl::surface) profile_image_data,
        (std::string) lobby_name,
        (banggame::game_options) options,
        (bool) allow_unofficial_expansions,
        (uint16_t) server_port
    )

    static inline const std::filesystem::path filename = std::filesystem::path(SDL_GetPrefPath(nullptr, "bang-sdl")) / "config.json";
    
    void load() {
        std::ifstream ifs{filename};
        if (ifs.fail()) return;
        try {
            Json::Value value;
            ifs >> value;
            *this = json::deserialize<config>(value);
            if (!profile_image.empty() && !profile_image_data) {
                profile_image_data = widgets::profile_pic::scale_profile_image(sdl::surface(resource(profile_image)));
            }
        } catch (const Json::RuntimeError &) {
            // ignore
        } catch (const std::runtime_error &) {
            profile_image.clear();
            profile_image_data.reset();
        }
    }

    void save() {
        std::ofstream ofs{filename};
        ofs << json::serialize(*this);
    }
};

#endif