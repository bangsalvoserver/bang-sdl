#include "config.h"

static const std::filesystem::path filename = std::filesystem::path(SDL_GetPrefPath(nullptr, "bang-sdl")) / "config.json";

void config::load() {
    std::ifstream ifs{filename};
    if (ifs.fail()) {
        return;
    }
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

void config::save()  {
    std::ofstream ofs{filename};
    ofs << json::serialize(*this);
}