#ifndef __FISTFULOFCARDS_SCENARIOS_H__
#define __FISTFULOFCARDS_SCENARIOS_H__

#include "../card_effect.h"

#include "../valleyofshadows/requests.h"

#include "../base/requests.h"

namespace banggame {

    struct effect_ambush {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_sniper {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_startofturn : effect_empty {
        opt_error verify(card *origin_card, player *origin) const;
    };

    struct effect_deadman {
        void on_enable(card *target_card, player *target);
    };

    struct effect_judge {
        void on_enable(card *target_card, player *target);
    };

    struct effect_lasso {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_abandonedmine {
        void on_enable(card *target_card, player *target);
    };

    struct effect_peyote : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_peyote : selection_picker {
        request_peyote(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct effect_ricochet {
        void on_play(card *origin_card, player *origin, card *target);
    };
    
    struct request_ricochet : request_destroy, missable_request {
        using request_destroy::request_destroy;

        game_formatted_string status_text(player *owner) const override;
    };
    
    struct effect_russianroulette  {
        void on_enable(card *target_card, player *target);
    };

    struct effect_fistfulofcards : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_lawofthewest : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_vendetta : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

}

#endif