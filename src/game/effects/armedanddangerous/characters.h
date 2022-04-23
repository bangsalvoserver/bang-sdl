#ifndef __ARMEDANDDANGEROUS_CHARACTERS_H__
#define __ARMEDANDDANGEROUS_CHARACTERS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_julie_cutter : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_frankie_canton {
        opt_error verify(card *origin_card, player *origin, card *target) const;
        void on_play(card *origin_card, player *origin, card *target);
    };

    struct effect_bloody_mary : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_red_ringo : event_based_effect {
        void on_equip(card *target_card, player *target);

        opt_error verify(card *origin_card, player *origin, card *target) const;
        void on_play(card *origin_card, player *origin, card *target);
    };

    struct effect_al_preacher : event_based_effect {
        void on_enable(card *target_card, player *target);

        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_ms_abigail : event_based_effect {
        bool can_escape(player *origin, card *origin_card, effect_flags flags) const;

        bool can_respond(card *origin_card, player *target) const;
        void on_play(card *origin_card, player *origin);

        void on_enable(card *origin_card, player *origin);
    };

}

#endif