#ifndef __REQUESTS_VALLEYOFSHADOWS_H__
#define __REQUESTS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {

    struct request_destroy : request_base, resolvable_request {
        request_destroy(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = {})
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}

        card *target_card;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_steal : request_base, resolvable_request {
        request_steal(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = {})
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}
        
        card *target_card;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_bandidos : request_base, resolvable_request {
        using request_base::request_base;

        int num_cards = 2;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_tornado : request_base {
        using request_base::request_base;
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_poker : request_base {
        using request_base::request_base;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_poker_draw : selection_picker {
        request_poker_draw(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct timer_damaging : timer_request {
        timer_damaging(card *origin_card, player *origin, player *target, int damage, bool is_bang)
            : timer_request(origin_card, origin, target, {}, 140)
            , damage(damage)
            , is_bang(is_bang) {}

        ~timer_damaging();
        
        int damage;
        bool is_bang;

        std::function<void()> cleanup_function;

        void on_finished() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_saved : request_base {
        request_saved(card *origin_card, player *target, player *saved)
            : request_base(origin_card, nullptr, target)
            , saved(saved) {}

        player *saved = nullptr;

        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct timer_lemonade_jim : request_base {
        using request_base::request_base;
        game_formatted_string status_text(player *owner) const override;
    };

}

#endif