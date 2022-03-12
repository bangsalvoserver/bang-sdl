#ifndef __EFFECTS_VALLEYOFSHADOWS_H__
#define __EFFECTS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_aim : card_effect {
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_backfire : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bandidos : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_tornado : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_poker : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_saved : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_escape : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct handler_fanning : card_effect {
        void verify(card *origin_card, player *origin, mth_target_list targets) const;
        void on_play(card *origin_card, player *origin, mth_target_list targets);
    };
}

#endif