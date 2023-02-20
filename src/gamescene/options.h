#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "widgets/defaults.h"
#include "sdl_wrap.h"

#include <fstream>
#include <sstream>
#include <chrono>

namespace banggame {
    
    using anim_duration_type = std::chrono::duration<float, duration_type::period>;

    extern const struct options_t {
        int card_width;

        int card_suit_offset;       // size of card suit icon
        float card_suit_scale;      // scale of card suit icon for resized texture

        int card_pocket_width;      // max width or card pocket
        int card_pocket_xoff;       // horizontal margin between cards in pocket
        int card_pocket_yoff;       // vertical distance from the center of player view to pocket

        int player_view_height;     // height of player view bounding rect

        int one_hp_size;            // size of "bullet" for one hp
        int character_offset;       // distance between cards in diagonal pockets

        int gold_yoffset;           // vertical distance from center of character card to top of gold text

        int deck_xoffset;           // horizontal distance from center of window to main deck
        int discard_xoffset;        // horizontal distance between main deck and discard pile
        
        int shop_xoffset;

        int shop_selection_width;   // max size of shop selection pocket

        int shop_choice_width;      // max size of shop choice pocket (for bottle/pardner)
        int shop_choice_offset;     // vertical distance from shop selection to shop choice

        int selection_yoffset;      // vertical distance from center of screen to selection pocket
        int selection_width;        // max size of selection pocket

        int cube_pile_size;         // size of square of cube pile
        int cube_pile_xoffset;      // distance from center of window to cube pile

        int cube_xdiff;             // horizontal distance from center of card to cube
        int cube_ydiff;             // vertical distance from center of card to first cube
        int cube_yoff;              // vertical distance between cubes

        int player_ellipse_x_distance;  // from border of screen
        int player_ellipse_y_distance;

        int default_border_thickness;   // card border thickness

        float easing_exponent;      // for card animations

        int card_margin;            // margin between cards
        int role_yoff;              // vertical distance between character card and role card
        int propic_yoff;            // vertical distance between role card and profile picture
        int dead_propic_yoff;       // vertical distance between role card and profile picture for dead players
        int username_yoff;          // vertical distance between profile picture and center of username

        float move_cubes_offset;

        float shuffle_deck_offset;

        int status_text_y_distance;
        int icon_dead_players_yoff;
        int pile_dead_players_xoff;
        int pile_dead_players_yoff;
        int pile_dead_players_ydiff;

        anim_duration_type card_overlay_duration; // how long you need to hold the mouse still
    } options;

    extern const struct colors_t {
        sdl::color flash_card;
        
        sdl::color status_text_background;

        sdl::color player_view_border;

        sdl::color turn_indicator;
        sdl::color request_origin_indicator;
        sdl::color request_target_indicator;
        sdl::color winner_indicator;
        sdl::color icon_dead_players;

        sdl::color target_finder_origin_card;
        sdl::color target_finder_highlight_card;
        sdl::color target_finder_current_card;
        sdl::color target_finder_target;
        sdl::color target_finder_can_confirm;
        sdl::color target_finder_can_respond;
        sdl::color target_finder_can_pick;
        sdl::color target_finder_picked;
    } colors;
}

#endif