#include "options.h"

namespace banggame {

    using namespace std::literals::chrono_literals;

    const options_t options {
        .card_width = 60,

        .card_suit_offset = 15,
        .card_suit_scale = 1.5f,

        .card_pocket_width = 180,
        .card_pocket_xoff = 10,
        .card_pocket_yoff = 55,

        .player_view_height = 220,

        .one_hp_size = 18,
        .card_diag_offset { 20, 20 },

        .gold_yoffset = 60,

        .deck_offset { 200, 30 },
        .discard_offset { -70, 0 },

        .scenario_offset { 70, 0 },

        .shop_deck_offset { -210, 50 },
        .shop_selection_offset { 140, 0 },

        .shop_selection_width = 150,

        .train_deck_offset { -210, -120 },
        .stations_offset { 65, 0},

        .train_offset { 0, 60 },
        .train_card_offset { 65, 0 },

        .card_choice_xoffset = 5,
        .card_choice_yoffset = 20,

        .selection_offset { 0, 0 },
        .selection_width = 300,

        .cube_pile_size = 50,
        .cube_pile_offset { 65, 25},

        .cube_xdiff = -20,
        .cube_ydiff = -25,
        .cube_yoff = 12,

        .player_ellipse_x_distance = 250,
        .player_ellipse_y_distance = 155,

        .button_row_yoffset = 40,

        .default_border_thickness = 5,

        .easing_exponent = 1.8f,

        .card_margin = 10,
        .role_yoff = 16,
        .propic_yoff = 94,
        .dead_propic_yoff = 15,
        .username_yoff = 40,

        .move_cubes_offset = 0.2f,

        .shuffle_deck_offset = 0.6f,

        .status_text_y_distance = 270,
        .icon_dead_players_yoff = 10,
        .pile_dead_players_xoff = 70,
        .pile_dead_players_yoff = 85,
        .pile_dead_players_ydiff = 70,

        .card_overlay_duration {1000ms}
    };

    const colors_t colors {
        .flash_card = sdl::rgba(0xffff00c0),

        .status_text_background = sdl::rgba(0xffffff80),

        .player_view_border = sdl::rgba(0x2d1000ff),

        .turn_indicator = sdl::rgba(0x4d7f21ff),
        .request_origin_indicator = sdl::rgba(0x7bf7ffff),
        .request_target_indicator = sdl::rgba(0xff0000ff),
        .winner_indicator = sdl::rgba(0xbba14fff),
        .icon_dead_players = sdl::rgba(0xdedbd3ff),

        .target_finder_origin_card = sdl::rgba(0x7bf7ffd0),
        .target_finder_highlight_card = sdl::rgba(0xff0000aa),
        .target_finder_current_card = sdl::rgba(0x306effee),
        .target_finder_target = sdl::rgba(0xff0000aa),
        .target_finder_targetable_card = sdl::rgba(0xffff0060),
        .target_finder_targetable_cube = sdl::rgba(0xffa000ff),
        .target_finder_targetable_player = sdl::rgba(0x806c00ff),
        .target_finder_can_confirm = sdl::rgba(0x0000ff80),
        .target_finder_can_play = sdl::rgba(0x1ed760aa),
        .target_finder_can_pick = sdl::rgba(0xffffffaa),
        .target_finder_picked = sdl::rgba(0x40c0ffaa)
    };

}