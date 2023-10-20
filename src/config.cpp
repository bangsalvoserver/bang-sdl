#include "config.h"

static const std::filesystem::path filename = std::filesystem::path(SDL_GetPrefPath(nullptr, "bang-sdl")) / "config.json";

std::vector<std::string> config::default_server_list() {
#ifdef OFFICIAL_BANG_SERVER
    return { OFFICIAL_BANG_SERVER };
#else
    return {};
#endif
}

void config::load() {
    std::ifstream ifs{filename};
    if (ifs.fail()) {
        return;
    }
    try {
        json::json value;
        ifs >> value;
        *this = json::deserialize<config>(value);
        if (!profile_image.empty() && !profile_image_data) {
            profile_image_data = widgets::profile_pic::scale_profile_image(sdl::surface(resource(profile_image)));
        }
    } catch (const std::exception &) {
        // ignore
    }
}

void config::save()  {
    std::ofstream ofs{filename};
    ofs << std::setw(2) << json::serialize(*this);
}