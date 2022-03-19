#ifndef __EFFECTS_CANYONDIABLO_H__
#define __EFFECTS_CANYONDIABLO_H__

#include "card_effect.h"

namespace banggame {

    struct effect_graverobber {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_mirage {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_disarm {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct handler_card_sharper {
        void verify(card *origin_card, player *origin, const mth_target_list &targets) const;
        void on_play(card *origin_card, player *origin, const mth_target_list &targets);
        void on_resolve(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card);
    };

    struct effect_sacrifice {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_lastwill {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct handler_lastwill {
        void on_play(card *origin_card, player *origin, const mth_target_list &targets);
    };

}

#endif