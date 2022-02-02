#ifndef __REQUESTS_BASE_H__
#define __REQUESTS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct request_predraw : request_base {
        request_predraw(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        game_formatted_string status_text() const;
    };

    struct request_draw : request_base, allowed_piles<card_pile_type::main_deck> {
        request_draw(player *target)
            : request_base(nullptr, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_check : request_base, allowed_piles<card_pile_type::selection> {
        request_check(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_generalstore : request_base, allowed_piles<card_pile_type::selection> {
        request_generalstore(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_discard : request_base, allowed_piles<card_pile_type::player_hand> {
        request_discard(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        int ncards = 1;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_discard_pass : request_base, allowed_piles<card_pile_type::player_hand> {
        player *next_player;
        
        request_discard_pass(player *target, player *next_player)
            : request_base(nullptr, nullptr, target)
            , next_player(next_player) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_indians : request_base {
        using request_base::request_base;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_duel : request_base {
        request_duel(card *origin_card, player *origin, player *target, player *respond_to, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , respond_to(respond_to) {}

        player *respond_to = nullptr;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_bang : request_base {
        using request_base::request_base;

        std::vector<card *> barrels_used;
        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        std::function<void()> cleanup_function;

        void on_resolve();
        void cleanup();
        game_formatted_string status_text() const;
    };

    struct request_death : request_base {
        request_death(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        std::vector<card *> draw_attempts;
        
        void on_resolve();
        game_formatted_string status_text() const;
    };

}

#endif