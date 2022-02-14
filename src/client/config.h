#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "utils/json_serial.h"
#include "utils/resource.h"

#include "common/card_enums.h"

#include "user_info.h"

struct config {
    REFLECTABLE(
        (std::vector<std::string>) recent_servers,
        (std::string) user_name,
        (std::string) profile_image,
        (std::vector<std::byte>) profile_image_data,
        (std::string) lobby_name,
        (banggame::card_expansion_type) expansions
    )

    static inline const std::filesystem::path filename = std::filesystem::path(SDL_GetPrefPath(nullptr, "bang-sdl")) / "config.json";

    config() = default;
    
    void load() {
        std::ifstream ifs{filename};
        if (ifs.fail()) return;
        try {
            Json::Value value;
            ifs >> value;
            *this = json::deserialize<config>(value);
            if (!profile_image.empty() && profile_image_data.empty()) {
                profile_image_data = encode_profile_image(sdl::surface(resource(profile_image)));
            }
        } catch (const Json::RuntimeError &error) {
            // ignore
        } catch (const std::runtime_error &e) {
            profile_image.clear();
            profile_image_data.clear();
        }
    }

    void save() {
        std::ofstream ofs{filename};
        ofs << json::serialize(*this);
    }
};

#endif