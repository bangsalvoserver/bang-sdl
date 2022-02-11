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

    void effect_add_gold::on_play(card *origin_card, player *origin, player *target) {
        target->add_gold(std::max<int>(args, 1));
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
        origin->m_game->m_next_in_turn = origin;
        origin->pass_turn();
    }
}