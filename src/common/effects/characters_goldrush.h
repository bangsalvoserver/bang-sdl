#ifndef __CHARACTERS_GOLDRUSH_H__
#define __CHARACTERS_GOLDRUSH_H__

#include "card_effect.h"

namespace banggame {

    struct effect_don_bell : event_based_effect {
        void on_equip(player *origin, card *target_card);
    };

    struct effect_madam_yto : event_based_effect {
        void on_equip(player *origin, card *target_card);
    };

    struct effect_greygory_deck : event_based_effect {
        void on_play(card *origin_card, player *origin);
        void on_equip(player *target, card *target_card);
    };

    struct effect_dutch_will : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_dutch_will : request_base, allowed_piles<card_pile_type::selection> {
        request_dutch_will(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_josh_mccloud : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct request_shop_choose_target : request_base, allowed_piles<card_pile_type::player> {
        request_shop_choose_target(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };
}

#endif