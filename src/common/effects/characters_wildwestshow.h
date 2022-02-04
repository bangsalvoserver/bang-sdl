#ifndef __CHARACTERS_WILDWESTSHOW_H__
#define __CHARACTERS_WILDWESTSHOW_H__

#include "card_effect.h"

namespace banggame {

    struct effect_big_spencer : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_gary_looter : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_john_pain : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_teren_kill : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_youl_grinner : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_youl_grinner : request_base {
        request_youl_grinner(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_flint_westwood_choose : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_flint_westwood : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

}

#endif