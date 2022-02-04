#ifndef __CHARACTERS_DODGECITY_H__
#define __CHARACTERS_DODGECITY_H__

#include "card_effect.h"

namespace banggame {
    
    struct effect_bill_noface : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_tequila_joe : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_claus_the_saint : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_claus_the_saint : request_base, allowed_piles<card_pile_type::selection> {
        request_claus_the_saint(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_greg_digger : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_herb_hunter : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_johnny_kisch : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_molly_stark : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_bellestar : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_vera_custer : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct request_vera_custer : request_base, allowed_piles<card_pile_type::player_character> {
        request_vera_custer(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };
}

#endif