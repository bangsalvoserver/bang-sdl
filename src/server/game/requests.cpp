#include "common/requests.h"
#include "common/effects.h"
#include "common/net_enums.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void request_predraw::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_table && args.player_id == target->id) {
            if (auto *check = target->get_if_top_predraw_check(args.card_id)) {
                target->m_game->pop_request();
                target->m_game->draw_check_then(target, check->check_fun);
            }
        }
    }

    void request_check::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::selection) {
            if (invert_pop_req) {
                target->m_game->resolve_check(args.card_id);
                target->m_game->pop_request();
            } else {
                target->m_game->pop_request();
                target->m_game->resolve_check(args.card_id);
            }
        }
    }

    void request_generalstore::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::selection) {
            auto next = target->m_game->get_next_player(target);
            auto removed = target->m_game->draw_from_temp(args.card_id);
            if (target->m_game->m_selection.size() == 1) {
                target->add_to_hand(std::move(removed));
                next->add_to_hand(std::move(target->m_game->m_selection.front()));
                target->m_game->m_selection.clear();
                target->m_game->pop_request();
            } else {
                target->m_game->pop_request_noupdate();
                target->add_to_hand(std::move(removed));
                target->m_game->queue_request<request_type::generalstore>(origin_card_id, origin, next);
            }
        }
    }

    void request_discard::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
            target->m_game->pop_request();
            target->discard_card(args.card_id);
        }
    }

    void request_discard_pass::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
            target->discard_card(args.card_id);
            target->m_game->instant_event<event_type::on_discard_pass>(target, args.card_id);
            if (target->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                target->m_game->queue_event<event_type::delayed_action>([target = this->target]{
                    if (target->can_receive_cubes()) {
                        target->m_game->queue_request<request_type::add_cube>(0, nullptr, target);
                    }
                });
            }
            if (target->num_hand_cards() <= target->max_cards_end_of_turn()) {
                target->m_game->pop_request();
                target->end_of_turn();
            }
        }
    }

    void request_damaging::on_resolve() {
        target->m_game->pop_request();
        target->damage(origin_card_id, origin, 1);
    }
    
    void timer_damaging::on_finished() {
        if (target->m_hp <= damage) {
            target->m_game->pop_request_noupdate();
        } else {
            target->m_game->pop_request();
        }
        target->do_damage(origin_card_id, origin, damage, is_bang);
    }

    void request_bang::on_resolve() {
        target->m_game->pop_request();
        target->damage(origin_card_id, origin, bang_damage, is_bang_card);
    }

    void request_bang::cleanup() {
        if (cleanup_function) {
            cleanup_function();
        }
    }

    void request_death::on_resolve() {
        target->m_game->player_death(target);
        target->m_game->pop_request();
    }

    void request_destroy::on_resolve() {
        target->m_game->pop_request();
        effect_destroy{}.on_play(origin_card_id, origin, target, card_id);
        target->m_game->queue_event<event_type::on_effect_end>(origin);
    }

    void request_steal::on_resolve() {
        target->m_game->pop_request();
        effect_steal{}.on_play(origin_card_id, origin, target, card_id);
        target->m_game->queue_event<event_type::on_effect_end>(origin);
    }

    void request_bandidos::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
            auto &moved = target->discard_card(args.card_id);
            if (--target->m_game->top_request().get<request_type::bandidos>().num_cards == 0
                || target->num_hand_cards() == 0
                || (escapable && !moved.responses.empty() && moved.responses.front().is(effect_type::escape))) {
                target->m_game->pop_request();
            }
        }
    }

    void request_tornado::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
            auto &moved = target->discard_card(args.card_id);
            if (!escapable || moved.responses.empty() || !moved.responses.front().is(effect_type::escape)) {
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
            }
            target->m_game->pop_request();
        }
    }

    void request_poker::on_pick(const pick_card_args &args) {
        if (origin != target) {
            if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
                auto it = std::ranges::find(target->m_hand, args.card_id, &card::id);

                if (escapable && !it->responses.empty() && it->responses.front().is(effect_type::escape)) {
                    target->discard_card(args.card_id);
                } else {
                    target->m_game->move_to(std::move(*it), card_pile_type::selection, true, origin);
                    target->m_hand.erase(it);
                }
                
                auto next = target;
                do {
                    next = target->m_game->get_next_player(next);
                } while (next->m_hand.empty() && next != origin);
                if (next == origin) {
                    if (std::ranges::any_of(target->m_game->m_selection, [](const deck_card &c) {
                        return c.value == card_value_type::value_A;
                    })) {
                        target->m_game->pop_request();
                        for (auto &c : target->m_game->m_selection) {
                            target->m_game->move_to(std::move(c), card_pile_type::discard_pile);
                        }
                        target->m_game->m_selection.clear();
                    } else if (target->m_game->m_selection.size() <= 2) {
                        for (auto &c : target->m_game->m_selection) {
                            next->add_to_hand(std::move(c));
                        }
                        target->m_game->m_selection.clear();
                        target->m_game->pop_request();
                    } else {
                        target->m_game->pop_request_noupdate();
                        target->m_game->queue_request<request_type::poker>(origin_card_id, origin, next);
                    }
                } else {
                    target->m_game->pop_request_noupdate();
                    target->m_game->queue_request<request_type::poker>(origin_card_id, origin, next);
                }
            }
        } else {
            if (args.pile == card_pile_type::selection) {
                target->add_to_hand(target->m_game->draw_from_temp(args.card_id));
                if (--target->m_game->top_request().get<request_type::poker>().num_cards == 0
                    || target->m_game->m_selection.size() == 0) {
                    target->m_game->pop_request();
                    for (auto &c : target->m_game->m_selection) {
                        target->m_game->move_to(std::move(c), card_pile_type::discard_pile);
                    }
                    target->m_game->m_selection.clear();
                }
            }
        }
    }

    void request_saved::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::main_deck) {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request();
        } else if (args.pile == card_pile_type::player_hand && args.player_id == saved->id) {
            for (int i=0; i<2 && !saved->m_hand.empty(); ++i) {
                target->steal_card(saved, saved->random_hand_card().id);
            }
            target->m_game->pop_request();
        }
    }

    void request_add_cube::on_pick(const pick_card_args &args) {
        auto &ncubes_ref = target->m_game->top_request().get<request_type::add_cube>().ncubes;
        if (args.player_id == target->id) {
            if (args.pile == card_pile_type::player_table) {
                auto &card = target->find_card(args.card_id);
                if (card.color == card_color_type::orange && card.cubes.size() < 4) {
                    target->add_cubes(card, 1);
                    --ncubes_ref;
                }
            } else if (args.pile == card_pile_type::player_character) {
                auto &card = target->m_characters.front();
                if (card.id == args.card_id && card.cubes.size() < 4) {
                    target->add_cubes(card, 1);
                    --ncubes_ref;
                }
            }
        }
        if (ncubes_ref == 0 || !target->can_receive_cubes()) {
            target->m_game->pop_request();
        }
    }

    void request_move_bomb::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player) {
            auto card_it = std::ranges::find(target->m_table, origin_card_id, &card::id);
            player *p = target->m_game->get_player(args.player_id);
            if (p && !p->immune_to(*card_it)) {
                if (p == target) {
                    target->m_game->pop_request();
                } else if (!p->has_card_equipped(card_it->name)) {
                    card_it->on_unequip(target);
                    p->equip_card(std::move(*card_it));
                    target->m_table.erase(card_it);
                    target->m_game->pop_request();
                }
            } else {
                target->discard_card(origin_card_id);
                target->m_game->pop_request();
            }
        }
    }

    void request_rust::on_resolve() {
        target->m_game->pop_request();

        effect_rust{}.on_play(origin_card_id, origin, target);
    }
}