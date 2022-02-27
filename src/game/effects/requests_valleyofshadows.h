#ifndef __REQUESTS_VALLEYOFSHADOWS_H__
#define __REQUESTS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {

    struct request_destroy : request_base {
        request_destroy(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}

        card *target_card;

        void on_resolve();
        game_formatted_string status_text(player *owner) const;
    };

    struct request_steal : request_base {
        request_steal(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}
        
        card *target_card;

        void on_resolve();
        game_formatted_string status_text(player *owner) const;
    };

    struct request_bandidos : request_base {
        using request_base::request_base;

        int num_cards = 2;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        void on_resolve();
        game_formatted_string status_text(player *owner) const;
    };

    struct request_tornado : request_base {
        using request_base::request_base;
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct request_poker : request_base {
        using request_base::request_base;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct request_poker_draw : selection_picker {
        request_poker_draw(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct timer_damaging : request_base, timer_request {
        timer_damaging(card *origin_card, player *origin, player *target, int damage, bool is_bang)
            : request_base(origin_card, origin, target)
            , timer_request(140)
            , damage(damage)
            , is_bang(is_bang) {}
        
        int damage;
        bool is_bang;

        std::function<void()> cleanup_function;

        void on_finished();
        void cleanup();
        game_formatted_string status_text(player *owner) const;
    };

    struct request_saved : request_base {
        request_saved(card *origin_card, player *target, player *saved)
            : request_base(origin_card, nullptr, target)
            , saved(saved) {}

        player *saved = nullptr;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };
    
    struct request_ricochet : request_destroy {
        using request_destroy::request_destroy;

        std::vector<card *> barrels_used;
        game_formatted_string status_text(player *owner) const;
    };

    struct timer_lemonade_jim : request_base, timer_request {
        using request_base::request_base;
        game_formatted_string status_text(player *owner) const;
    };

}

#endif