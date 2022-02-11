#ifndef __SCENARIOS_FISTFULOFCARDS_H__
#define __SCENARIOS_FISTFULOFCARDS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_ambush : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_sniper : card_effect {
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

    struct effect_lasso : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_abandonedmine : scenario_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_peyote : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_peyote : request_base, allowed_piles<card_pile_type::selection> {
        request_peyote(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_ricochet : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
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