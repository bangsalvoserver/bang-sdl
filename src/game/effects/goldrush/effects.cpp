#include "effects.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_sell_beer::on_play(card *origin_card, player *origin, card *target_card) {
        origin->m_game->add_log("LOG_SOLD_BEER", origin, target_card);
        origin->discard_card(target_card);
        origin->add_gold(1);
        origin->m_game->call_event<event_type::on_play_beer>(origin);
    }

    opt_error effect_discard_black::verify(card *origin_card, player *origin, card *target_card) const {
        if (origin->m_gold < target_card->buy_cost() + 1) {
            return game_error("ERROR_NOT_ENOUGH_GOLD");
        }
        return std::nullopt;
    }

    void effect_discard_black::on_play(card *origin_card, player *origin, card *target_card) {
        origin->m_game->add_log("LOG_DISCARDED_CARD", origin, target_card->owner, target_card);
        origin->add_gold(-target_card->buy_cost() - 1);
        target_card->owner->discard_card(target_card);
    }

    void effect_add_gold::on_play(card *origin_card, player *origin, player *target) {
        target->add_gold(amount);
    }

    bool effect_pay_gold::can_respond(card *origin_card, player *origin) const {
        return origin->m_gold >= amount;
    }

    opt_error effect_pay_gold::verify(card *origin_card, player *origin) const {
        if (origin->m_gold < amount) {
            return game_error("ERROR_NOT_ENOUGH_GOLD");
        }
        return std::nullopt;
    }

    void effect_pay_gold::on_play(card *origin_card, player *origin) {
        origin->add_gold(-amount);
    }

    void effect_rum::on_play(card *origin_card, player *origin) {
        std::vector<card_suit> suits;
        for (int i=0; i < 3 + origin->m_num_checks; ++i) {
            origin->m_game->add_log("LOG_REVEALED_CARD", origin, origin->m_game->m_deck.back());
            suits.push_back(origin->get_card_sign(origin->m_game->draw_card_to(pocket_type::selection)).suit);
        }
        while (!origin->m_game->m_selection.empty()) {
            card *drawn_card = origin->m_game->m_selection.front();
            origin->m_game->call_event<event_type::on_draw_check>(origin, drawn_card);
            if (drawn_card->pocket == pocket_type::selection) {
                origin->m_game->move_card(drawn_card, pocket_type::discard_pile);
            }
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
        ++origin->m_extra_turns;
    }
}