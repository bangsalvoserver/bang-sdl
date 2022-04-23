#ifndef __DODGECITY_CHARACTERS_H__
#define __DODGECITY_CHARACTERS_H__

#include "../card_effect.h"

namespace banggame {
    
    struct effect_bill_noface : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_tequila_joe : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_claus_the_saint : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_claus_the_saint : selection_picker {
        request_claus_the_saint(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        player *get_next_target() const;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct effect_greg_digger : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_herb_hunter : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_johnny_kisch : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_molly_stark : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_bellestar {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_vera_custer : event_based_effect {
        void on_enable(card *target_card, player *target);

        static void copy_characters(player *origin, player *target);
        static void remove_characters(player *origin);
    };

    struct request_vera_custer : request_base {
        request_vera_custer(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct handler_doc_holyday {
        void on_play(card *origin_card, player *origin, const target_list &targets);
    };
}

#endif