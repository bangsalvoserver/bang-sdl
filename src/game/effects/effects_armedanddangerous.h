#ifndef __EFFECTS_ARMEDANDDANGEROUS_H__
#define __EFFECTS_ARMEDANDDANGEROUS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_draw_atend : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_pay_cube : card_effect {
        void verify(card *origin_card, player *origin) const {
            verify(origin_card, origin, origin, origin_card);
        }

        void verify(card *origin_card, player *origin, player *target, card *target_card) const;

        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin, origin_card);
        }

        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_add_cube : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };
    
    struct effect_reload : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_rust : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_bandolier : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_belltower : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_doublebarrel : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_thunderer : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_buntlinespecial : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bigfifty : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_flintlock : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_duck : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_squaw_destroy : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_squaw_steal : card_effect {
        void on_play(card *origin_card, player *origin);
    };
}

#endif