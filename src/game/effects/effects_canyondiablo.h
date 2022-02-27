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

    struct handler_card_sharper {
        void verify(card *origin_card, player *origin, mth_target_list targets) const;
        void on_play(card *origin_card, player *origin, mth_target_list targets);
        void on_resolve(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card);
    };

    struct effect_sacrifice : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_lastwill : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct handler_lastwill {
        void on_play(card *origin_card, player *origin, mth_target_list targets);
    };

}

#endif