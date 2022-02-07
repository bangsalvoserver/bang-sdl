#ifndef __SCENARIOS_HIGHNOON_H__
#define __SCENARIOS_HIGHNOON_H__

#include "card_effect.h"

namespace banggame {

    struct effect_blessing : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_curse : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_thedaltons : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_thedaltons : request_base {
        request_thedaltons(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_thedoctor : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_trainarrival : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_thirst : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_highnoon : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_shootout : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_invert_rotation : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_reverend : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_hangover : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_sermon : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_ghosttown : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_handcuffs : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_handcuffs : request_base, allowed_piles<card_pile_type::selection> {
        request_handcuffs(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_shopchoice : request_base {
        request_shopchoice(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        game_formatted_string status_text() const;
    };

    struct effect_newidentity : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_newidentity : request_base {
        request_newidentity(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };
}

#endif