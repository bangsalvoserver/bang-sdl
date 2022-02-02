#ifndef __CHARACTERS_BASE_H__
#define __CHARACTERS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_black_jack : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_calamity_janet : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_slab_the_killer : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_kit_carlson : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_kit_carlson : request_base, allowed_piles<card_pile_type::selection> {
        request_kit_carlson(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_el_gringo : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_suzy_lafayette : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_vulture_sam : event_based_effect {
        void on_equip(player *target, card *target_card);
    };
}

#endif