#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "utils/json_serial.h"
#include "utils/resource.h"

#include "common/card_enums.h"

#include "user_info.h"

struct config {
private:
    std::string m_filename;

public:
    REFLECTABLE(
        (std::vector<std::string>) recent_servers,
        (std::string) user_name,
        (std::string) profile_image,
        (std::vector<std::byte>) profile_image_data,
        (std::string) lobby_name,
        (banggame::card_expansion_type) expansions
    )

    config() = default;

    config(const std::string &filename) {
        std::ifstream ifs(filename);
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
        m_filename = filename;
    }

    void save() {
        std::ofstream ofs(m_filename);
        ofs << json::serialize(*this);
    }
};

#endif