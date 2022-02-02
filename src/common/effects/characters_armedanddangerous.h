#ifndef __CHARACTERS_ARMEDANDDANGEROUS_H__
#define __CHARACTERS_ARMEDANDDANGEROUS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_julie_cutter : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_frankie_canton : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_bloody_mary : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_red_ringo : event_based_effect {
        void on_equip(player *target, card *target_card);

        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_al_preacher : event_based_effect {
        void on_equip(player *target, card *target_card);

        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_ms_abigail : card_effect {
        bool can_escape(player *origin, card *origin_card, effect_flags flags) const;

        bool can_respond(card *origin_card, player *target) const;
        void on_play(card *origin_card, player *origin);
    };

}

#endif