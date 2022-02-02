#ifndef __CHARACTERS_WILDWESTSHOW_H__
#define __CHARACTERS_WILDWESTSHOW_H__

#include "card_effect.h"

namespace banggame {

    struct effect_big_spencer : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_gary_looter : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_john_pain : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_teren_kill : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_youl_grinner : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_youl_grinner : request_base, allowed_piles<card_pile_type::player_hand> {
        request_youl_grinner(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

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