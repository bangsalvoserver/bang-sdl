#include "common/effects/scenarios_highnoon.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_ambush::on_equip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            p.add_player_flags(player_flags::disable_player_distances);
        }
    }

    void effect_ambush::on_unequip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            p.remove_player_flags(player_flags::disable_player_distances);
        }
    }

    void effect_sniper::on_play(card *origin_card, player *origin, player *target) {
        request_bang req{origin_card, origin, target, flags};
        req.bang_strength = 2;
        target->m_game->queue_request(std::move(req));
    }

    void effect_startofturn::verify(card *origin_card, player *origin) const {
        if (!origin->check_player_flags(player_flags::start_of_turn)) {
            throw game_error("ERROR_NOT_START_OF_TURN");
        }
    }

    void effect_deadman::on_equip(card *target_card, player *target) {
        target->m_game->m_scenario_flags |= scenario_flags::deadman;
    }

    void effect_judge::on_equip(card *target_card, player *target) {
        target->m_game->m_scenario_flags |= scenario_flags::judge;
    }

    void effect_lasso::on_equip(card *target_card, player *target) {
        target->m_game->add_disabler(target_card, [](card *c) {
            return c->pile == card_pile_type::player_table;
        });
    }

    void effect_lasso::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
    }

    void effect_abandonedmine::on_equip(card *target_card, player *target) {
        target->m_game->m_scenario_flags |= scenario_flags::abandonedmine;
    }

    void effect_peyote::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_request_draw>(target_card, [=](player *p) {
            auto &vec = p->m_game->m_hidden_deck;
            for (auto it = vec.begin(); it != vec.end(); ) {
                auto *card = *it;
                if (!card->responses.empty() && card->responses.front().is(effect_type::peyotechoice)) {
                    it = p->m_game->move_to(card, card_pile_type::selection, true, nullptr, show_card_flags::no_animation);
                } else {
                    ++it;
                }
            }
            
            p->m_game->queue_request<request_type::peyote>(target_card, p);
        });
    }

    void request_peyote::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto *drawn_card = target->m_game->m_deck.back();
        target->m_game->send_card_update(*drawn_card, nullptr, show_card_flags::short_pause);

        if ((target_card->responses.front().args == 1)
            ? (drawn_card->suit == card_suit_type::hearts || drawn_card->suit == card_suit_type::diamonds)
            : (drawn_card->suit == card_suit_type::clubs || drawn_card->suit == card_suit_type::spades))
        {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
        } else {
            target->m_game->draw_card_to(card_pile_type::discard_pile);

            while (!target->m_game->m_selection.empty()) {
                target->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
            }
            target->m_game->pop_request(request_type::peyote);
            target->m_game->queue_event<event_type::post_draw_cards>(target);
        }
    }

    game_formatted_string request_peyote::status_text() const {
        return {"STATUS_PEYOTE", origin_card};
    }

    void effect_ricochet::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->m_game->queue_request<request_type::ricochet>(origin_card, origin, target, target_card, flags);
    }

    game_formatted_string request_ricochet::status_text() const {
        return {"STATUS_RICOCHET", origin_card, target_card};
    }

    void effect_russianroulette::on_equip(card *target_card, player *target) {
        auto queue_russianroulette_request = [=](player *target) {
            request_bang req{target_card, nullptr, target};
            req.bang_damage = 2;
            target->m_game->queue_request(std::move(req));
        };
        queue_russianroulette_request(target);
        target->m_game->add_event<event_type::on_missed>(target_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (target_card == origin_card) {
                queue_russianroulette_request(target->m_game->get_next_player(target));
            }
        });
        target->m_game->add_event<event_type::on_hit>(target_card, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (target_card == origin_card) {
                target->m_game->remove_events(target_card);
            }
        });
    }

    void effect_fistfulofcards::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            for (int i=0; i<p->m_hand.size(); ++i) {
                p->m_game->queue_request<request_type::bang>(target_card, nullptr, p);
            }
        });
    }
}