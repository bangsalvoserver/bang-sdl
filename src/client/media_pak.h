#ifndef __MEDIA_PAK_H__
#define __MEDIA_PAK_H__

#include "utils/sdl.h"
#include "utils/resource.h"

#include <filesystem>

class media_pak {
public:
    sdl::surface icon_bang;

    sdl::texture texture_background;

    sdl::texture icon_checkbox;
    sdl::texture icon_default_user;
    sdl::texture icon_disconnected;
    sdl::texture icon_loading;
    sdl::texture icon_owner;

    sdl::texture icon_turn;
    sdl::texture icon_origin;
    sdl::texture icon_target;
    sdl::texture icon_winner;

    sdl::texture icon_gold;

    sdl::texture sprite_cube;
    sdl::texture sprite_cube_border;

    resource font_arial;
    resource font_perdido;
    resource font_bkant_bold;

    static const media_pak &get() {
        return *s_instance;
    }

public:
    media_pak(const std::filesystem::path &base_path);

private:
    static inline media_pak *s_instance = nullptr;
};

#endif