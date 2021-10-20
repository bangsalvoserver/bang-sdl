#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <vector>

#include "utils/json_serial.h"
#include "common/card_enums.h"

DEFINE_SERIALIZABLE(config_data,
    (recent_servers, std::vector<std::string>)
    (user_name, std::string)
    (lobby_name, std::string)
    (expansions, banggame::card_expansion_type)
)

class config : public config_data {
private:
    std::string m_filename;

public:
    config(const std::string &filename) : config_data([&]{
        std::ifstream ifs(filename);
        try {
            Json::Value value;
            ifs >> value;
            return deserialize(value);
        } catch (const Json::RuntimeError &error) {
            return config_data{};
        }
    }())
    , m_filename(filename) {}

    void save() {
        std::ofstream ofs(m_filename);
        ofs << serialize();
    }
};

#endif