#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include "../card_enums.h"
#include "../game_action.h"
#include "../format_str.h"

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
        void on_unequip(card *target_card, player *target);
    };

    struct predraw_check_effect : card_effect {
        void on_unequip(card *target_card, player *target);
    };

    struct request_base {
        request_base(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags)
            : origin_card(origin_card), origin(origin), target(target), flags(flags) {}

        card *origin_card;
        player *origin;
        player *target;
        effect_flags flags;
    };

    struct timer_request {
        timer_request(int duration = 200) : duration(duration) {}

        int duration;
    };

    template<card_pile_type ... Es>
    struct allowed_piles {
        bool can_pick(card_pile_type pile, player *, card *) const {
            return ((pile == Es) || ...);
        }
    };

    struct scenario_effect : card_effect {
        void on_unequip(card *target_card, player *target) {}
    };

}


#endif