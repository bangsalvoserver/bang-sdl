#ifndef __OPTIONS_H__
#define __OPTIONS_H__

namespace banggame::options {
    constexpr int card_width = 60;
    constexpr int card_xoffset = 10;
    constexpr int card_yoffset = 55;

    constexpr int player_hand_width = 180;
    constexpr int player_view_height = 220;

    constexpr int one_hp_size = 18;
    constexpr int character_offset = 20;

    constexpr int gold_yoffset = 60;

    constexpr int deck_xoffset = 200;
    constexpr int shop_xoffset = 10;

    constexpr int discard_xoffset = 70;
    
    constexpr int selection_yoffset = 100;
    constexpr int selection_width = 300;

    constexpr int shop_selection_width = 150;

    constexpr int shop_choice_width = 130;
    constexpr int shop_choice_offset = 20;

    constexpr int cube_pile_size = 50;
    constexpr int cube_pile_xoffset = 70;

    constexpr int cube_xdiff = -20;
    constexpr int cube_ydiff = -25;
    constexpr int cube_yoff = 12;

    constexpr int scenario_deck_xoff = 40;

    constexpr int player_ellipse_x_distance = 250;
    constexpr int player_ellipse_y_distance = 180;

    constexpr int card_overlay_timer = 60;

    constexpr int default_border_thickness = 5;

    constexpr int status_text_y_distance = 45;
    constexpr uint32_t status_text_background_rgba = 0xffffff80;

    constexpr uint32_t player_view_border_rgba = 0x2d1000ff;

    constexpr uint32_t turn_indicator_rgba = 0x4d7f21ff;
    constexpr uint32_t request_origin_indicator_rgba = 0x7bf7ffff;
    constexpr uint32_t request_target_indicator_rgba = 0xff0000ff;
    constexpr uint32_t winner_indicator_rgba = 0xbba14fff;

    constexpr uint32_t target_finder_current_card_rgba = 0x306effee;
    constexpr uint32_t target_finder_target_rgba = 0xff0000aa;
    constexpr uint32_t target_finder_can_respond_rgba = 0x1ed760aa;
    constexpr uint32_t target_finder_can_pick_rgba = 0xffffffaa;
}

#endif