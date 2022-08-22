#include "options.h"

namespace banggame {

    const options_t options = {
        .card_width = 60,
        .card_xoffset = 10,
        .card_yoffset = 55,

        .card_suit_offset = 15,
        .card_suit_scale = 1.5f,

        .player_hand_width = 180,
        .player_view_height = 220,

        .one_hp_size = 18,
        .character_offset = 20,

        .gold_yoffset = 60,

        .deck_xoffset = 200,
        .shop_xoffset = 10,

        .discard_xoffset = 70,

        .selection_yoffset = 10,
        .selection_width = 300,

        .shop_selection_width = 150,

        .shop_choice_width = 130,
        .shop_choice_offset = 20,

        .cube_pile_size = 50,
        .cube_pile_xoffset = 70,

        .cube_xdiff = -20,
        .cube_ydiff = -25,
        .cube_yoff = 12,

        .scenario_deck_xoff = 40,

        .player_ellipse_x_distance = 250,
        .player_ellipse_y_distance = 155,

        .card_overlay_msecs = 1000,

        .default_border_thickness = 5,

        .easing_exponent = 1.8f,

        .card_margin = 10,
        .role_yoff = 16,
        .propic_yoff = 94,
        .username_yoff = 43,
        .dead_propic_yoff = 12,

        .move_card_msecs = 333,
        .flip_card_msecs = 167,
        .short_pause_msecs = 333,
        .tap_card_msecs = 167,
        .flash_card_msecs = 167,
        .move_hp_msecs = 333,
        .flip_role_msecs = 250,
        .shuffle_deck_msecs = 1333,

        .move_cube_msecs = 133,
        .move_cubes_msecs = 250,
        .move_cubes_offset = 0.2f,

        .shuffle_deck_offset = 0.5f,

        .status_text_y_distance = 270,
        .icon_dead_players_yoff = 10,
        .pile_dead_players_yoff = 80,
        
        .flash_card_color = sdl::rgba(0xffff00c0),

        .status_text_background = sdl::rgba(0xffffff80),

        .player_view_border = sdl::rgba(0x2d1000ff),

        .turn_indicator = sdl::rgba(0x4d7f21ff),
        .request_origin_indicator = sdl::rgba(0x7bf7ffff),
        .request_target_indicator = sdl::rgba(0xff0000ff),
        .winner_indicator = sdl::rgba(0xbba14fff),
        .icon_dead_players = sdl::rgba(0xdedbd3ff),

        .target_finder_origin_card = sdl::rgba(0x7bf7ffd0),
        .target_finder_current_card = sdl::rgba(0x306effee),
        .target_finder_target = sdl::rgba(0xff0000aa),
        .target_finder_can_respond = sdl::rgba(0x1ed760aa),
        .target_finder_can_pick = sdl::rgba(0xffffffaa),
        .target_finder_picked = sdl::rgba(0xc0ffffaa)
    };

}