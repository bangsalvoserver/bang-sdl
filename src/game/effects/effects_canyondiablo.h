#ifndef __EFFECTS_CANYONDIABLO_H__
#define __EFFECTS_CANYONDIABLO_H__

#include "card_effect.h"

namespace banggame {

    struct effect_graverobber : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_mirage : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_disarm : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct card_sharper_handler {
        card *origin_card;
        player *origin;
        player *target;
        card *chosen_card;
        card *target_card;

        void operator()(player *p, card *c);
        void on_resolve();
    };

    struct effect_card_sharper_choose : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_card_sharper_switch : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_sacrifice : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

}

#endif