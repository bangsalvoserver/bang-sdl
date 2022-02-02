#include "common/effects/effects_goldrush.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_sell_beer::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->discard_card(target_card);
        origin->add_gold(1);
        origin->m_game->add_log("LOG_SOLD_BEER", origin, target_card);
        origin->m_game->queue_event<event_type::on_play_beer>(origin);
        origin->m_game->queue_event<event_type::on_effect_end>(origin, target_card);
    }

    void effect_discard_black::verify(card *origin_card, player *origin, player *target, card *target_card) {
        if (origin->m_gold < target_card->buy_cost + 1) {
            throw game_error("ERROR_NOT_ENOUGH_GOLD");
        }
    }

    void effect_discard_black::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->add_gold(-target_card->buy_cost - 1);
        target->discard_card(target_card);
        origin->m_game->add_log("LOG_DISCARDED_CARD", origin, target, target_card);
    }

    void effect_rum::on_play(card *origin_card, player *origin) {
        std::vector<card_suit_type> suits;
        for (int i=0; i < 3 + origin->m_num_checks; ++i) {
            suits.push_back(origin->get_card_suit(origin->m_game->draw_card_to(card_pile_type::selection)));
        }
        while (!origin->m_game->m_selection.empty()) {
            card *drawn_card = origin->m_game->m_selection.front();
            origin->m_game->move_to(drawn_card, card_pile_type::discard_pile);
            origin->m_game->queue_event<event_type::on_draw_check>(origin, drawn_card);
        }
        std::sort(suits.begin(), suits.end());
        origin->heal(std::unique(suits.begin(), suits.end()) - suits.begin());
    }

    void effect_goldrush::on_play(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::on_turn_end>(origin_card, [=](player *p) {
            if (p == origin) {
                origin->heal(origin->m_max_hp);
                origin->m_game->remove_events(origin_card);
            }
        });
        origin->pass_turn(origin);
    }

    static void swap_shop_choice_in(card *origin_card, player *origin, effect_type type) {
        while (!origin->m_game->m_shop_selection.empty()) {
            origin->m_game->move_to(origin->m_game->m_shop_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }

        auto &vec = origin->m_game->m_hidden_deck;
        for (auto it = vec.begin(); it != vec.end(); ) {
            auto *card = *it;
            if (!card->responses.empty() && card->responses.front().is(type)) {
                it = origin->m_game->move_to(card, card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
            } else {
                ++it;
            }
        }

        origin->m_game->queue_request<request_type::shopchoice>(origin_card, origin);
    }

    void effect_bottle::on_play(card *origin_card, player *origin) {
        swap_shop_choice_in(origin_card, origin, effect_type::bottlechoice);
    }

    void effect_pardner::on_play(card *origin_card, player *origin) {
        swap_shop_choice_in(origin_card, origin, effect_type::pardnerchoice);
    }

    bool effect_shopchoice::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::shopchoice, origin);
    }

    void effect_shopchoice::on_play(card *origin_card, player *origin) {
        int n_choice = origin->m_game->m_shop_selection.size();
        while (!origin->m_game->m_shop_selection.empty()) {
            origin->m_game->move_to(origin->m_game->m_shop_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }

        auto it = origin->m_game->m_hidden_deck.end() - n_choice - 2;
        for (int i=0; i<2; ++i) {
            it = origin->m_game->move_to(*it, card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
        }
        origin->m_game->pop_request(request_type::shopchoice);
        origin->m_game->queue_event<event_type::delayed_action>([m_game = origin->m_game]{
            while (m_game->m_shop_selection.size() < 3) {
                m_game->draw_shop_card();
            }
        });
    }

    game_formatted_string request_shopchoice::status_text() const {
        return {"STATUS_SHOPCHOICE", origin_card};
    }
}