#ifndef __SCENARIOS_H__
#define __SCENARIOS_H__

#include "card_effect.h"

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

    struct request_thedaltons : request_base, allowed_piles<card_pile_type::player_table> {
        request_thedaltons(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
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

    struct effect_shootout : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_invert_rotation : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_reverend : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_hangover : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_sermon : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_ghosttown : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_ambush : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_lasso : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_fistfulofcards : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_judge : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_peyote : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_peyote : request_base, allowed_piles<card_pile_type::selection> {
        request_peyote(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_handcuffs : event_based_effect {
        void on_equip(player *target, card *target_card);
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
    
    struct effect_russianroulette : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_abandonedmine : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_deadman : scenario_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_startofturn : effect_empty {
        void verify(card *origin_card, player *origin) const;
    };

    struct effect_sniper : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_ricochet : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };
}

#endif