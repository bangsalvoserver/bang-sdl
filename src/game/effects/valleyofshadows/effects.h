#ifndef __VALLEYOFSHADOWS_EFFECTS_H__
#define __VALLEYOFSHADOWS_EFFECTS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_aim {
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_backfire {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bandidos {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_tornado {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_poker {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_saved {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_escape {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct handler_fanning {
        void verify(card *origin_card, player *origin, const mth_target_list &targets) const;
        void on_play(card *origin_card, player *origin, const mth_target_list &targets);
    };
}

#endif