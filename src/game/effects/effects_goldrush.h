#ifndef __EFFECTS_GOLDRUSH__
#define __EFFECTS_GOLDRUSH__

#include "card_effect.h"

namespace banggame {

    struct effect_sell_beer {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_discard_black {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_add_gold {
        int amount;
        effect_add_gold(int value) : amount(std::max(1, value)) {}

        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_pay_gold {
        int amount;

        void verify(card *origin_card, player *origin) const;
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_rum {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_goldrush {
        void on_play(card *origin_card, player *origin);
    };
}

#endif