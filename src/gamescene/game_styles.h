#ifndef __GAME_STYLES_H__
#define __GAME_STYLES_H__

#include "widgets/style_tracker.h"

namespace banggame {

    enum class game_style {
        current_turn,
        current_card,
        selected_target,
        targetable,
        playable,
        highlight,
        origin_card,
        pickable,
        picked
    };

    using game_style_set = widgets::style_set<game_style>;
    using game_style_tracker = widgets::style_tracker<game_style>;
    
    inline sdl::color cube_border_color(game_style style) {
        switch (style) {
        case game_style::targetable: return colors.target_finder_targetable_cube;
        case game_style::selected_target: return colors.target_finder_target;
        default: return {};
        }
    }

    inline sdl::color card_border_color(game_style style) {
        switch (style) {
        case game_style::current_card: return colors.target_finder_current_card;
        case game_style::selected_target: return colors.target_finder_target;
        case game_style::playable: return colors.target_finder_can_play;
        case game_style::highlight: return colors.target_finder_highlight_card;
        case game_style::origin_card: return colors.target_finder_origin_card;
        case game_style::targetable: return colors.target_finder_targetable_card;
        case game_style::pickable: return colors.target_finder_can_pick;
        case game_style::picked: return colors.target_finder_picked;
        default: return {};
        }
    }

    inline sdl::color player_border_color(game_style style) {
        switch (style) {
        case game_style::current_turn: return colors.turn_indicator;
        case game_style::selected_target: return colors.target_finder_target;
        case game_style::targetable: return colors.target_finder_targetable_player;
        default: return {};
        }
    }

    inline sdl::color button_toggle_color(game_style style) {
        switch (style) {
        case game_style::current_card: return colors.game_ui_button_down;
        case game_style::playable: return colors.game_ui_button_playable;
        default: return {};
        }
    }

}

#endif