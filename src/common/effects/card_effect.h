#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include "common/card_enums.h"
#include "common/game_action.h"
#include "common/format_str.h"

#include <ranges>

namespace banggame {

    struct player;
    struct card;

    struct card_effect {
        target_type target = enums::flags_none<target_type>;
        short args = 0;
        effect_flags flags = no_effect_flags;
    };

    struct effect_empty : card_effect {
        void on_play(card *origin_card, player *origin) {}
    };

    struct event_based_effect : card_effect {
        void on_unequip(player *target, card *target_card);
    };

    struct predraw_check_effect : card_effect {
        void on_unequip(player *target, card *target_card);
    };

    struct request_base {
        request_base(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags)
            : origin_card(origin_card), origin(origin), target(target), flags(flags) {}

        card *origin_card;
        player *origin;
        player *target;
        effect_flags flags;
    };

    struct timer_base : request_base {
        timer_base(card *origin_card, player *origin, player *target, int duration = 200)
            : request_base(origin_card, origin, target)
            , duration(duration) {}

        int duration;
    };

    template<card_pile_type ... Es>
    struct allowed_piles {
        static bool valid_pile(card_pile_type pile) {
            return ((pile == Es) || ...);
        }
    };

}


#endif