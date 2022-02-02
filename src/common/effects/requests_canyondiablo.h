#ifndef __REQUESTS_CANYONDIABLO_H__
#define __REQUESTS_CANYONDIABLO_H__

#include "card_effect.h"

namespace banggame {
    
    struct request_card_sharper : request_base {
        request_card_sharper(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card)
            : request_base(origin_card, origin, target, effect_flags::escapable)
            , chosen_card(chosen_card)
            , target_card(target_card) {}

        card *chosen_card;
        card *target_card;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_lastwill : request_base, allowed_piles<card_pile_type::player_hand, card_pile_type::player_table> {
        request_lastwill(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        int ncards = 3;

        void on_resolve();
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_lastwill_target : request_base, allowed_piles<card_pile_type::player> {
        request_lastwill_target(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

}


#endif