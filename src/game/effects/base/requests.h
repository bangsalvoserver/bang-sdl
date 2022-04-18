#ifndef __BASE_REQUESTS_H__
#define __BASE_REQUESTS_H__

#include "../card_effect.h"

namespace banggame {

    struct request_characterchoice : request_base {
        request_characterchoice(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_predraw : request_base {
        request_predraw(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_draw : request_base {
        request_draw(player *target)
            : request_base(nullptr, nullptr, target) {}

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_check : selection_picker {
        request_check(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_generalstore : selection_picker {
        request_generalstore(card *origin_card, player *origin, player *target)
            : selection_picker(origin_card, origin, target) {}

        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_discard : request_base {
        request_discard(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        int ncards = 1;
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_discard_pass : request_base {
        request_discard_pass(player *target)
            : request_base(nullptr, nullptr, target) {}

        int ndiscarded = 0;

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_indians : request_base, resolvable_request {
        using request_base::request_base;

        bool can_pick(pocket_type pocket, player *target_player, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target_player, card *target_card) override;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct request_duel : request_base, resolvable_request {
        request_duel(card *origin_card, player *origin, player *target, player *respond_to, effect_flags flags = {})
            : request_base(origin_card, origin, target, flags)
            , respond_to(respond_to) {}

        player *respond_to = nullptr;

        bool can_pick(pocket_type pocket, player *target_player, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target_player, card *target_card) override;

        void on_resolve() override;
        game_formatted_string status_text(player *owner) const override;
    };

    class missable_request {
    public:
        void add_card(card *c) {
            m_cards_used.push_back(c);
        }

        virtual bool can_respond(card *c) const {
            return std::ranges::find(m_cards_used, c) == m_cards_used.end();
        }

        virtual void on_miss(player *origin);

    private:
        std::vector<card *> m_cards_used;
    };

    struct request_bang : request_base, missable_request, cleanup_request, resolvable_request {
        using request_base::request_base;

        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        bool can_respond(card *c) const override;
        void on_miss(player *origin) override;
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