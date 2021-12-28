#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "common/json_serial.h"
#include "common/card_enums.h"

struct config {
private:
    std::string m_filename;

public:
    REFLECTABLE(
        (std::vector<std::string>) recent_servers,
        (std::string) user_name,
        (std::string) profile_image,
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
        } catch (const Json::RuntimeError &error) {}
        m_filename = filename;
    }

    void save() {
        std::ofstream ofs(m_filename);
        ofs << json::serialize(*this);
    }
};

#endif