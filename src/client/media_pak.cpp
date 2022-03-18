#include "media_pak.h"

#include "utils/unpacker.h"

media_pak::media_pak(const std::filesystem::path &base_path) {
    auto media_pak_stream = ifstream_or_throw(base_path / "media.pak");
    unpacker media_pak(media_pak_stream);

    icon_bang =                          sdl::surface(media_pak["icon_bang"]);

    texture_background =    sdl::texture(sdl::surface(media_pak["background"]));

    icon_checkbox =         sdl::texture(sdl::surface(media_pak["icon_checkbox"]));
    icon_default_user =     sdl::texture(sdl::surface(media_pak["icon_default_user"]));
    icon_disconnected =     sdl::texture(sdl::surface(media_pak["icon_disconnected"]));
    icon_loading =          sdl::texture(sdl::surface(media_pak["icon_loading"]));
    icon_owner =            sdl::texture(sdl::surface(media_pak["icon_owner"]));

    icon_turn =             sdl::texture(sdl::surface(media_pak["icon_turn"]));
    icon_origin =           sdl::texture(sdl::surface(media_pak["icon_origin"]));
    icon_target =           sdl::texture(sdl::surface(media_pak["icon_target"]));
    icon_winner =           sdl::texture(sdl::surface(media_pak["icon_winner"]));

    icon_dead_players =     sdl::texture(sdl::surface(media_pak["icon_dead_players"]));

    icon_gold =             sdl::texture(sdl::surface(media_pak["icon_gold"]));

    sprite_cube =           sdl::texture(sdl::surface(media_pak["sprite_cube"]));
    sprite_cube_border =    sdl::texture(sdl::surface(media_pak["sprite_cube_border"]));

    font_arial =            media_pak["arial"];
    font_perdido =          media_pak["perdido"];
    font_bkant_bold =       media_pak["bkant_bold"];

    s_instance = this;
}