#ifndef __SCENARIOS_FISTFULOFCARDS_H__
#define __SCENARIOS_FISTFULOFCARDS_H__

#include "card_effect.h"
#include "requests_valleyofshadows.h"
#include "requests_base.h"

namespace banggame {

    struct effect_ambush {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_sniper {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_startofturn : effect_empty {
        void verify(card *origin_card, player *origin) const;
    };

    struct effect_deadman : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_judge : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_lasso {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_abandonedmine : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_peyote : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_peyote : selection_picker {
        request_peyote(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct effect_ricochet {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };
    
    struct request_ricochet : request_destroy, barrel_ptr_vector {
        using request_destroy::request_destroy;

        game_formatted_string status_text(player *owner) const;
    };
    
    struct effect_russianroulette : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_fistfulofcards : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_lawofthewest : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_vendetta : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

}

#endif