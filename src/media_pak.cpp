#include "media_pak.h"

#include "utils/unpacker.h"

media_pak::media_pak(const std::filesystem::path &base_path, sdl::renderer &renderer) {
    auto media_pak_stream = ifstream_or_throw(base_path / "media.pak");
    unpacker media_pak(media_pak_stream);

    icon_bang = sdl::surface(media_pak["icon_bang"]);

    font_arial =            media_pak["fonts/arial"];
    font_perdido =          media_pak["fonts/perdido"];
    font_bkant_bold =       media_pak["fonts/bkant_bold"];

    texture_background =    sdl::texture(renderer, media_pak["background"]);

    icon_checkbox =         sdl::texture(renderer, media_pak["icon_checkbox"]);
    icon_default_user =     sdl::texture(renderer, media_pak["icon_default_user"]);
    icon_disconnected =     sdl::texture(renderer, media_pak["icon_disconnected"]);
    icon_loading =          sdl::texture(renderer, media_pak["icon_loading"]);
    icon_owner =            sdl::texture(renderer, media_pak["icon_owner"]);

    icon_turn =             sdl::texture(renderer, media_pak["icon_turn"]);
    icon_origin =           sdl::texture(renderer, media_pak["icon_origin"]);
    icon_target =           sdl::texture(renderer, media_pak["icon_target"]);
    icon_winner =           sdl::texture(renderer, media_pak["icon_winner"]);

    icon_dead_players =     sdl::texture(renderer, media_pak["icon_dead_players"]);

    icon_gold =             sdl::texture(renderer, media_pak["icon_gold"]);

    sprite_cube =           sdl::texture(renderer, media_pak["sprite_cube"]);
    sprite_cube_border =    sdl::texture(renderer, media_pak["sprite_cube_border"]);

    s_instance = this;
}