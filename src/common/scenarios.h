#ifndef __SCENARIOS_H__
#define __SCENARIOS_H__

#include "effects.h"
#include "equips.h"

namespace banggame {
    DEFINE_ENUM_FLAGS_IN_NS(banggame, scenario_flags,
        (invert_rotation) // inverti giro
        (reverend) // annulla birra
        (hangover) // annulla personaggio
        (sermon) // annulla bang
        (ghosttown) // citta' fantasma
        (ambush) // setta distanze a 1
        (lasso) // annulla carte in gioco
        (judge) // non si puo' equipaggiare
    )

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

    struct effect_ambush : scenario_effect {
        void on_equip(player *target, card *target_card);
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

    struct request_peyote : picking_request_allowing<card_pile_type::selection> {
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct effect_handcuffs : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_handcuffs : picking_request_allowing<card_pile_type::selection> {
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };
}

#endif