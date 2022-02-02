#ifndef __EFFECTS_GOLDRUSH__
#define __EFFECTS_GOLDRUSH__

#include "card_effect.h"

namespace banggame {

    struct effect_sell_beer : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_discard_black : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card);
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_rum : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_goldrush : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bottle : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_pardner : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_shopchoice : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
}

#endif