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
        sdl::point card_diag_offset; // distance between cards in diagonal pockets

        int gold_yoffset;           // vertical distance from center of character card to top of gold text

        sdl::point deck_offset;     // distance from center of window to main deck
        sdl::point discard_offset;  // distance from main deck and discard pile

        sdl::point scenario_offset; // distance from main deck to scenario card pile 
        
        sdl::point shop_deck_offset; // distance from center of window to shop deck
        sdl::point shop_selection_offset; // distance from shop deck to shop selection center

        int shop_selection_width;   // max size of shop selection pocket

        sdl::point train_deck_offset; // distance from center of window to train deck
        sdl::point stations_offset; // distance from train deck to stations

        sdl::point train_offset;
        sdl::point train_card_offset; // horizontal padding between cards in train

        int card_choice_xoffset;    // horizontal offset between card choices
        int card_choice_yoffset;    // vertical distance from anchor to card choices

        sdl::point selection_offset; // distance from center of screen to selection pocket
        int selection_width;        // max size of selection pocket

        int cube_pile_size;         // size of square of cube pile
        sdl::point cube_pile_offset;      // distance from center of window to cube pile

        int cube_xdiff;             // horizontal distance from center of card to cube
        int cube_ydiff;             // vertical distance from center of card to first cube
        int cube_yoff;              // vertical distance between cubes

        int player_ellipse_x_distance;  // from border of screen
        int player_ellipse_y_distance;

        int button_row_yoffset;

        int default_border_thickness;   // card border thickness

        float easing_exponent;      // for card animations
        float flash_exponent;

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
        sdl::color target_finder_targetable_card;
        sdl::color target_finder_targetable_cube;
        sdl::color target_finder_targetable_player;
        sdl::color target_finder_can_play;
        sdl::color target_finder_can_pick;
        sdl::color target_finder_picked;

        sdl::color game_ui_button_confirm;
        sdl::color game_ui_button_down;
        sdl::color game_ui_button_playable;
    } colors;
}

#endif