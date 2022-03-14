#ifndef __CHARACTERS_BASE_H__
#define __CHARACTERS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_black_jack : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_calamity_janet {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_slab_the_killer : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_kit_carlson : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_kit_carlson : selection_picker {
        request_kit_carlson(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct effect_el_gringo : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_suzy_lafayette : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_vulture_sam : event_based_effect {
        void on_equip(card *target_card, player *target);
    };
}

#endif