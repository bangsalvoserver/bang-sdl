#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "utils/reflector.h"
#include "utils/json_serial.h"

#include "utils/sdl.h"

#include <fstream>
#include <sstream>

DECLARE_RESOURCE(game_options_json)

#include <charconv>

namespace json {
    template<> struct deserializer<sdl::color> {
        sdl::color operator()(const Json::Value &value) const {
            if (value.isArray()) {
                return {
                    uint8_t(value[0].asInt()),
                    uint8_t(value[1].asInt()),
                    uint8_t(value[2].asInt()),
                    uint8_t(value.size() == 4 ? value[3].asInt() : 0xff)
                };
            } else if (value.isString()) {
                std::string str = value.asString();
                uint32_t ret;
                auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), ret, 16);
                if (ec == std::errc{} && ptr == str.data() + str.size()) {
                    if (value.size() == 6) {
                        return sdl::rgb(ret);
                    } else {
                        return sdl::rgba(ret);
                    }
                } else {
                    return {};
                }
            } else if (value.isInt()) {
                return sdl::rgba(value.asInt());
            } else {
                return {};
            }
        }
    };
}

namespace banggame {

    struct options_t {REFLECTABLE(
        (int) card_width,
        (int) card_xoffset,
        (int) card_yoffset,

        (int) card_suit_offset,
        (float) card_suit_scale,

        (int) player_hand_width,
        (int) player_view_height,

        (int) one_hp_size,
        (int) character_offset,

        (int) gold_yoffset,

        (int) deck_xoffset,
        (int) shop_xoffset,

        (int) discard_xoffset,
        
        (int) selection_yoffset,
        (int) selection_width,

        (int) shop_selection_width,

        (int) shop_choice_width,
        (int) shop_choice_offset,

        (int) cube_pile_size,
        (int) cube_pile_xoffset,

        (int) cube_xdiff,
        (int) cube_ydiff,
        (int) cube_yoff,

        (int) scenario_deck_xoff,

        (int) player_ellipse_x_distance,
        (int) player_ellipse_y_distance,

        (int) card_overlay_timer,

        (int) default_border_thickness,

        (float) easing_exponent,

        (int) card_margin,
        (int) role_yoff,
        (int) propic_yoff,
        (int) username_yoff,

        (int) move_card_ticks,
        (int) move_cube_ticks,
        (int) flip_card_ticks,
        (int) short_pause_ticks,
        (int) tap_card_ticks,
        (int) move_hp_ticks,
        (int) flip_role_ticks,
        (int) shuffle_deck_ticks,

        (float) shuffle_deck_offset,

        (int) status_text_y_distance,
        (int) icon_dead_players_yoff,
        (int) pile_dead_players_yoff,

        (sdl::color) status_text_background,

        (sdl::color) player_view_border,

        (sdl::color) turn_indicator,
        (sdl::color) request_origin_indicator,
        (sdl::color) request_target_indicator,
        (sdl::color) winner_indicator,
        (sdl::color) icon_dead_players,

        (sdl::color) target_finder_current_card,
        (sdl::color) target_finder_target,
        (sdl::color) target_finder_can_respond,
        (sdl::color) target_finder_can_pick
    )};

    static inline const options_t options = []{
        auto load_options = [](auto &&stream) {
            Json::Value json_value;
            stream >> json_value;
            return json::deserialize<options_t>(json_value);
        };
        if (auto stream = std::ifstream("resources/game_options.json")) {
            return load_options(stream);
        } else {
            auto options_res = GET_RESOURCE(game_options_json);
            return load_options(std::istringstream(std::string(options_res.data, options_res.length)));
        }
    }();
}

#endif