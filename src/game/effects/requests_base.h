#ifndef __REQUESTS_BASE_H__
#define __REQUESTS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct request_characterchoice : request_base {
        request_characterchoice(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_predraw : request_base {
        request_predraw(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_draw : request_base {
        request_draw(player *target)
            : request_base(nullptr, nullptr, target) {}

        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_check : selection_picker {
        request_check(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_generalstore : selection_picker {
        request_generalstore(card *origin_card, player *origin, player *target)
            : selection_picker(origin_card, origin, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_discard : request_base {
        request_discard(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        int ncards = 1;
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_discard_pass : request_base {
        request_discard_pass(player *target)
            : request_base(nullptr, nullptr, target) {}

        bool can_pick(card_pile_type pile, player *target, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_indians : request_base, resolvable_request {
        using request_base::request_base;

        bool can_pick(card_pile_type pile, player *target_player, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target_player, card *target_card) override;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_duel : request_base, resolvable_request {
        request_duel(card *origin_card, player *origin, player *target, player *respond_to, effect_flags flags = {})
            : request_base(origin_card, origin, target, flags)
            , respond_to(respond_to) {}

        player *respond_to = nullptr;

        bool can_pick(card_pile_type pile, player *target_player, card *target_card) const override;
        void on_pick(card_pile_type pile, player *target_player, card *target_card) override;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct barrel_ptr_vector {
        std::vector<card *> barrels_used;
    };

    struct request_bang : request_base, barrel_ptr_vector, resolvable_request {
        using request_base::request_base;

        ~request_bang();

        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        std::function<void()> cleanup_function;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_death : request_base, resolvable_request {
        request_death(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        std::vector<card *> draw_attempts;
        
        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

}

#endif