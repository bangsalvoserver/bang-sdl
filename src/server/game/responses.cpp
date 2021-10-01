#include "common/responses.h"
#include "common/effects.h"
#include "common/net_enums.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void response_predraw::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_table) {
            auto &c = target->find_table_card(card_id);
            if (target->is_top_predraw_check(c)) {
                auto t = target;
                t->m_game->pop_response();
                for (auto &e : c.effects) {
                    e->on_predraw_check(t, card_id);
                }
            }
        }
    }

    void response_check::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            t->m_game->pop_response();
            t->m_game->resolve_check(card_id);
        }
    }

    void response_generalstore::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto o = origin;
            auto t = target;
            auto next = t->m_game->get_next_player(t);
            auto removed = t->m_game->draw_from_temp(card_id);
            if (t->m_game->m_temps.size() == 1) {
                t->m_game->pop_response();
                t->add_to_hand(std::move(removed));
                next->add_to_hand(std::move(t->m_game->m_temps.front()));
                t->m_game->m_temps.clear();
            } else {
                t->m_game->pop_response_noupdate();
                t->add_to_hand(std::move(removed));
                t->m_game->queue_response<response_type::generalstore>(o, next);
            }
        }
    }

    void response_discard::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            t->m_game->pop_response();
            t->discard_card(card_id);
            if (t->num_hand_cards() <= t->m_hp) {
                t->m_game->next_turn();
            }
        }
    }
    
    void response_duel::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto &target_card = target->find_hand_card(card_id);
            if (target_card.effects.front().is<effect_bangcard>()
                || target->has_character<effect_calamity_janet>() && target_card.effects.front().is<effect_missedcard>()) {
                auto o = origin;
                auto t = target;
                t->m_game->pop_response_noupdate();
                t->m_game->queue_response<response_type::duel>(t, o);
                t->discard_hand_card_response(card_id);
            }
        }
    }

    void response_duel::on_resolve() {
        auto o = origin;
        auto t = target;
        t->m_game->pop_response();
        t->damage(o, 1);
    }

    void response_indians::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            auto &target_card = t->find_hand_card(card_id);
            if (target_card.effects.front().is<effect_bangcard>()
                || target->has_character<effect_calamity_janet>() && target_card.effects.front().is<effect_missedcard>()) {
                t->m_game->pop_response();
                t->discard_hand_card_response(card_id);
            }
        }
    }

    void response_indians::on_resolve() {
        auto o = origin;
        auto t = target;
        t->m_game->pop_response();
        t->damage(o, 1);
    }
    
    void response_bang::handle_missed() {
        if (0 == --get_data()->bang_strength) {
            target->m_game->pop_response();
        }
    }

    void response_bang::handle_barrel(int card_id) {
        auto &vec = get_data()->barrels_used;
        if (std::ranges::find(vec, card_id) == std::ranges::end(vec)) {
            auto pos = std::ranges::find(vec, 0);
            if (pos == std::ranges::end(vec)) {
                throw game_error("Stack overflow barili");
            }
            *pos = card_id;
            target->m_game->draw_check_then(target, [this](card_suit_type suit, card_value_type) {
                if (suit == card_suit_type::hearts) {
                    handle_missed();
                }
            });
        }
    }

    void response_bang::on_respond(const play_card_args &args) {
        auto t = target;
        if (auto card_it = std::ranges::find(t->m_hand, args.card_id, &deck_card::id); card_it != t->m_hand.end()) {
            if (card_it->effects.front().is<effect_missed>()) {
                handle_missed();
                t->do_play_card(args.card_id, args.targets);
            }
        } else if (auto card_it = std::ranges::find(t->m_table, args.card_id, &deck_card::id); card_it != t->m_table.end()) {
            if (!t->m_game->table_cards_disabled(t->id)) {
                if (card_it->effects.front().is<effect_barrel>()) {
                    handle_barrel(args.card_id);
                } else if (card_it->effects.front().is<effect_missed>()) {
                    handle_missed();
                    t->do_play_card(args.card_id, args.targets);
                }
            } else {
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(t->m_characters, args.card_id, &character::id); card_it != t->m_characters.end()) {
            if (card_it->effects.front().is<effect_barrel>()) {
                handle_barrel(args.card_id);
            } else if (card_it->effects.front().is<effect_calamity_janet>()) {
                auto &c = t->find_hand_card(args.targets.front().get<play_card_target_type::target_card>().front().card_id);
                if (c.effects.front().is<effect_bangcard>()) {
                    handle_missed();
                    t->discard_hand_card_response(c.id);
                }
            } else if (card_it->effects.front().is<effect_elena_fuente>()) {
                handle_missed();
                t->discard_hand_card_response(args.targets.front().get<play_card_target_type::target_card>().front().card_id);
            }
        }
    }

    void response_bang::on_resolve() {
        auto o = origin;
        auto t = target;
        t->m_game->pop_response();
        t->damage(o, 1);
    }

    void response_death::on_respond(const play_card_args &args) {
        card &target_card = target->find_any_card(args.card_id);
        if (target_card.effects.front().is<effect_deathsave>()) {
            target->do_play_card(args.card_id, args.targets);
            if (target->m_hp > 0) {
                target->m_game->pop_response();
            }
        }
    }

    void response_death::on_resolve() {
        auto o = origin;
        auto t = target;
        t->handle_death();
        t->m_game->pop_response();
        t->m_game->player_death(o, t);
    }
}