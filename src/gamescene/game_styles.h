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

    struct game_style_set : widgets::style_set<game_style> {
        sdl::color border_color{};

        virtual sdl::color get_border_color_for(game_style style) = 0;

        void update_style() override {
            if (auto style = get_style()) {
                border_color = get_border_color_for(*style);
            } else {
                border_color = {};
            }
        }
    };

    using game_style_tracker = widgets::style_tracker<game_style>;

}

#endif