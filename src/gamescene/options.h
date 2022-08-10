#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "sdl_wrap.h"

#include <fstream>
#include <sstream>

namespace banggame {

    extern const struct options_t {
        int card_width;
        int card_xoffset;
        int card_yoffset;

        int card_suit_offset;
        float card_suit_scale;

        int player_hand_width;
        int player_view_height;

        int one_hp_size;
        int character_offset;

        int gold_yoffset;

        int deck_xoffset;
        int shop_xoffset;

        int discard_xoffset;
        
        int selection_yoffset;
        int selection_width;

        int shop_selection_width;

        int shop_choice_width;
        int shop_choice_offset;

        int cube_pile_size;
        int cube_pile_xoffset;

        int cube_xdiff;
        int cube_ydiff;
        int cube_yoff;

        int scenario_deck_xoff;

        int player_ellipse_x_distance;
        int player_ellipse_y_distance;

        int card_overlay_msecs;

        int default_border_thickness;

        float easing_exponent;

        int card_margin;
        int role_yoff;
        int propic_yoff;
        int username_yoff;

        int move_card_msecs;
        int flip_card_msecs;
        int short_pause_msecs;
        int tap_card_msecs;
        int move_hp_msecs;
        int flip_role_msecs;
        int shuffle_deck_msecs;
        
        int move_cube_msecs;
        int move_cubes_msecs;
        float move_cubes_offset;

        float shuffle_deck_offset;

        int status_text_y_distance;
        int icon_dead_players_yoff;
        int pile_dead_players_yoff;

        sdl::color status_text_background;

        sdl::color player_view_border;

        sdl::color turn_indicator;
        sdl::color request_origin_indicator;
        sdl::color request_target_indicator;
        sdl::color winner_indicator;
        sdl::color icon_dead_players;

        sdl::color target_finder_current_card;
        sdl::color target_finder_target;
        sdl::color target_finder_can_respond;
        sdl::color target_finder_can_pick;
    } options;
}

#endif