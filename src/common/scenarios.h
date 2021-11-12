#ifndef __SCENARIOS_H__
#define __SCENARIOS_H__

#include "effects.h"
#include "equips.h"

namespace banggame {
    struct scenario_effect : card_effect {
        void on_unequip(player *target, card *target_card) {}
    };

    struct effect_blessing : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_curse : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_thedaltons : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_thedaltons : picking_request_allowing<card_pile_type::player_table> {
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct effect_thedoctor : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_trainarrival : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_thirst : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_highnoon : event_based_effect {
        void on_equip(player *target, card *target_card);
    };
}

#endif