#include "common/requests.h"
#include "common/effects.h"
#include "common/net_enums.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void request_predraw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            if (auto *check = target->get_if_top_predraw_check(target_card)) {
                target->m_game->pop_request();
                target->m_game->draw_check_then(target, check->check_fun);
            }
        }
    }

    void request_check::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_game->pop_request();
        target->m_game->resolve_check(target, target_card);
    }

    void request_generalstore::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto next = target->m_game->get_next_player(target);
        if (target->m_game->m_selection.size() == 2) {
            target->add_to_hand(target_card);
            next->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request();
        } else {
            target->m_game->pop_request_noupdate();
            target->add_to_hand(target_card);
            target->m_game->queue_request<request_type::generalstore>(origin_card, origin, next);
        }
    }

    void request_discard::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            if (--target->m_game->top_request().get<request_type::discard>().ncards == 0) {
                target->m_game->pop_request();
            }

            target->discard_card(target_card);
            target->m_game->queue_event<event_type::on_effect_end>(target, origin_card);
        }
    }

    void request_discard_pass::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            target->m_game->instant_event<event_type::on_discard_pass>(target, target_card);
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
        target->damage(origin_card, origin, 1);
    }
    
    void timer_damaging::on_finished() {
        if (target->m_hp <= damage) {
            target->m_game->pop_request_noupdate();
        } else {
            target->m_game->pop_request();
        }
        target->do_damage(origin_card, origin, damage, is_bang);
    }

    void request_bang::on_resolve() {
        target->m_game->pop_request();
        target->damage(origin_card, origin, bang_damage, is_bang_card);
        cleanup();
    }

    void request_bang::cleanup() {
        if (cleanup_function) {
            cleanup_function();
            cleanup_function = nullptr;
        }
    }

    void request_death::on_resolve() {
        target->m_game->player_death(target);
        target->m_game->pop_request();
    }

    void request_destroy::on_resolve() {
        effect_destroy{}.on_play(origin_card, origin, target, m_target_card);
        target->m_game->pop_request();
    }

    void request_steal::on_resolve() {
        effect_steal{}.on_play(origin_card, origin, target, m_target_card);
        target->m_game->pop_request();
    }

    void request_bandidos::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            if (--target->m_game->top_request().get<request_type::bandidos>().num_cards == 0
                || target->num_hand_cards() == 0) {
                target->m_game->pop_request();
            }
        }
    }

    void request_bandidos::on_resolve() {
        target->m_game->pop_request();
        target->damage(origin_card, origin, 1);
    }

    void request_tornado::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request();
        }
    }

    void request_poker::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target == target_player) {
            target->m_game->move_to(target_card, card_pile_type::selection, true, origin);
            target->m_game->pop_request();
        }
    }

    void request_poker_draw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->add_to_hand(target_card);
        if (--target->m_game->top_request().get<request_type::poker_draw>().num_cards == 0
            || target->m_game->m_selection.size() == 0) {
            for (auto *c : target->m_game->m_selection) {
                target->m_game->move_to(c, card_pile_type::discard_pile);
            }
            target->m_game->pop_request();
        }
    }

    void request_saved::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (pile == card_pile_type::main_deck) {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request();
        } else if (pile == card_pile_type::player_hand && target_player == saved) {
            for (int i=0; i<2 && !saved->m_hand.empty(); ++i) {
                target->steal_card(saved, saved->random_hand_card());
            }
            target->m_game->pop_request();
        }
    }

    void request_add_cube::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target
            && ((pile == card_pile_type::player_table && target_card->color == card_color_type::orange)
            || (pile == card_pile_type::player_character && target_card == target->m_characters.front()))
            && target_card->cubes.size() < 4)
        {
            target->add_cubes(target_card, 1);
            if (--target->m_game->top_request().get<request_type::add_cube>().ncubes == 0 || !target->can_receive_cubes()) {
                target->m_game->pop_request();
            }
        }
    }

    void request_move_bomb::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (!target_player->immune_to(*origin_card)) {
            if (target_player == target) {
                target->m_game->pop_request();
            } else if (!target_player->has_card_equipped(origin_card->name)) {
                origin_card->on_unequip(target);
                target_player->equip_card(origin_card);
                target->m_game->pop_request();
            }
        } else {
            target->discard_card(origin_card);
            target->m_game->pop_request();
        }
    }

    void request_rust::on_resolve() {
        target->m_game->pop_request();

        effect_rust{}.on_play(origin_card, origin, target);
    }
}