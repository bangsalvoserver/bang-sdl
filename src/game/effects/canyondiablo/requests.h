#ifndef __CANYONDIABLO_REQUESTS_H__
#define __CANYONDIABLO_REQUESTS_H__

#include "../card_effect.h"

namespace banggame {
    
    struct request_card_sharper : request_base, resolvable_request {
        request_card_sharper(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card)
            : request_base(origin_card, origin, target, effect_flags::escapable)
            , chosen_card(chosen_card)
            , target_card(target_card) {}

        card *chosen_card;
        card *target_card;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

}


#endif