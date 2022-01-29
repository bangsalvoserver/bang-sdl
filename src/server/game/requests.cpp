#include "common/requests.h"
#include "common/effects.h"
#include "common/net_enums.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void request_predraw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            int top_priority = std::ranges::max(target->m_predraw_checks
                | std::views::values
                | std::views::filter(std::not_fn(&player::predraw_check::resolved))
                | std::views::transform(&player::predraw_check::priority));
            auto it = target->m_predraw_checks.find(target_card);
            if (it != target->m_predraw_checks.end()
                && !it->second.resolved
                && it->second.priority == top_priority) {
                target->m_game->pop_request(request_type::predraw);
                target->m_game->draw_check_then(target, target_card, it->second.check_fun);
            }
        }
    }

    void request_draw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->draw_from_deck();
    }

    game_formatted_string request_draw::status_text() const {
        return "STATUS_YOUR_TURN";
    }

    game_formatted_string request_predraw::status_text() const {
        using predraw_check_pair = decltype(player::m_predraw_checks)::value_type;
        auto unresolved = target->m_predraw_checks
            | std::views::filter([](const predraw_check_pair &pair) {
                return !pair.second.resolved;
            });
        auto top_priority = unresolved
            | std::views::filter([value = std::ranges::max(unresolved
                | std::views::values
                | std::views::transform(&player::predraw_check::priority))]
            (const predraw_check_pair &pair) {
                return pair.second.priority == value;
            });
        if (std::ranges::distance(top_priority) == 1) {
            return {"STATUS_PREDRAW_FOR", top_priority.begin()->first};
        } else {
            return "STATUS_PREDRAW";
        }
    }

    void request_check::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        while (!target->m_game->m_selection.empty()) {
            card *drawn_card = target->m_game->m_selection.front();
            target->m_game->move_to(drawn_card, card_pile_type::discard_pile);
            target->m_game->queue_event<event_type::on_draw_check>(target, drawn_card);
        }
        target->m_game->pop_request(request_type::check);
        target->m_game->add_log("LOG_CHECK_DREW_CARD", target->m_game->m_current_check->origin_card, target, target_card);
        target->m_game->instant_event<event_type::trigger_tumbleweed>(target->m_game->m_current_check->origin_card, target_card);
        if (!target->m_game->m_current_check->no_auto_resolve) {
            target->m_game->m_current_check->function(target_card);
            target->m_game->m_current_check.reset();
        }
    }

    game_formatted_string request_check::status_text() const {
        return {"STATUS_CHECK", origin_card};
    }

    void request_generalstore::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto next = target->m_game->get_next_player(target);
        if (target->m_game->m_selection.size() == 2) {
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card);
            target->add_to_hand(target_card);
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", next, target->m_game->m_selection.front());
            next->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request(request_type::generalstore);
        } else {
            target->m_game->pop_request_noupdate(request_type::generalstore);
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card);
            target->add_to_hand(target_card);
            target->m_game->queue_request<request_type::generalstore>(origin_card, origin, next);
        }
    }

    game_formatted_string request_generalstore::status_text() const {
        return "STATUS_GENERALSTORE";
    }

    void request_discard::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            if (--target->m_game->top_request().get<request_type::discard>().ncards == 0) {
                target->m_game->pop_request(request_type::discard);
            }

            target->discard_card(target_card);
            target->m_game->queue_event<event_type::on_effect_end>(target, origin_card);
        }
    }

    game_formatted_string request_discard::status_text() const {
        return {"STATUS_DISCARD", origin_card};
    }

    void request_discard_pass::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            if (target->m_game->has_scenario(scenario_flags::abandonedmine)) {
                target->move_card_to(target_card, card_pile_type::main_deck);
            } else {
                target->discard_card(target_card);
            }
            target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, target_card);
            target->m_game->instant_event<event_type::on_discard_pass>(target, target_card);
            if (target->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                target->m_game->queue_event<event_type::delayed_action>([target = this->target]{
                    if (target->can_receive_cubes()) {
                        target->m_game->queue_request<request_type::add_cube>(nullptr, target);
                    }
                });
            }
            if (target->num_hand_cards() <= target->max_cards_end_of_turn()) {
                target->m_game->pop_request(request_type::discard_pass);
                target->m_game->queue_event<event_type::delayed_action>([*this]{
                    target->end_of_turn(next_player);
                });
            }
        }
    }

    game_formatted_string request_discard_pass::status_text() const {
        return "STATUS_DISCARD_PASS";
    }

    void request_indians::on_resolve() {
        target->m_game->pop_request(request_type::indians);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_indians::status_text() const {
        return {"STATUS_INDIANS", origin_card};
    }

    void request_duel::on_resolve() {
        target->m_game->pop_request(request_type::duel);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_duel::status_text() const {
        return {"STATUS_DUEL", origin_card};
    }
    
    void timer_damaging::on_finished() {
        if (origin->m_hp <= damage) {
            origin->m_game->pop_request_noupdate(request_type::damaging);
        } else {
            origin->m_game->pop_request(request_type::damaging);
        }
        origin->do_damage(origin_card, source, damage, is_bang);
        cleanup();
    }

    void timer_damaging::cleanup() {
        if (cleanup_function) {
            cleanup_function();
            cleanup_function = nullptr;
        }
    }

    game_formatted_string timer_damaging::status_text() const {
        return {damage > 1 ? "STATUS_DAMAGING_PLURAL" : "STATUS_DAMAGING", origin, origin_card, damage};
    }

    void request_bang::on_resolve() {
        target->m_game->pop_request(request_type::bang);
        target->damage(origin_card, origin, bang_damage, is_bang_card);
        if (!target->m_game->m_requests.empty() && target->m_game->m_requests.back().is(request_type::damaging)) {
            auto &req = target->m_game->m_requests.back().get<request_type::damaging>();
            req.cleanup_function = std::move(cleanup_function);
        } else {
            cleanup();
        }
    }

    void request_bang::cleanup() {
        if (cleanup_function) {
            cleanup_function();
            cleanup_function = nullptr;
        }
    }

    game_formatted_string request_bang::status_text() const {
        if (unavoidable) {
            return {"STATUS_BANG_UNAVOIDABLE", origin_card};
        } else if (bang_strength > 1) {
            return {"STATUS_BANG_MULTIPLE_MISSED", origin_card, bang_strength};
        } else {
            return {"STATUS_BANG", origin_card};
        }
    }

    void request_death::on_resolve() {
        target->m_game->player_death(target);
        target->m_game->check_game_over(target);
        target->m_game->pop_request(request_type::death);
    }

    game_formatted_string request_death::status_text() const {
        return "STATUS_DEATH";
    }

    void request_destroy::on_resolve() {
        effect_destroy{}.on_play(origin_card, origin, target, target_card);
        target->m_game->pop_request(request_type::destroy);
    }

    game_formatted_string request_destroy::status_text() const {
        return {"STATUS_DESTROY", origin_card, with_owner{target_card}};
    }

    game_formatted_string request_ricochet::status_text() const {
        return {"STATUS_RICOCHET", origin_card, target_card};
    }

    void request_steal::on_resolve() {
        effect_steal{}.on_play(origin_card, origin, target, target_card);
        target->m_game->pop_request(request_type::steal);
    }

    game_formatted_string request_steal::status_text() const {
        return {"STATUS_STEAL", origin_card, with_owner{target_card}};
    }

    void request_bandidos::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            if (--target->m_game->top_request().get<request_type::bandidos>().num_cards == 0
                || target->num_hand_cards() == 0) {
                target->m_game->pop_request(request_type::bandidos);
            }
        }
    }

    void request_bandidos::on_resolve() {
        target->m_game->pop_request(request_type::bandidos);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_bandidos::status_text() const {
        return {"STATUS_BANDIDOS", origin_card};
    }

    void request_tornado::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request(request_type::tornado);
        }
    }

    game_formatted_string request_tornado::status_text() const {
        return {"STATUS_TORNADO", origin_card};
    }

    void request_poker::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target == target_player) {
            target->m_game->move_to(target_card, card_pile_type::selection, true, origin);
            target->m_game->pop_request(request_type::poker);
        }
    }

    game_formatted_string request_poker::status_text() const {
        return {"STATUS_POKER", origin_card};
    }

    void request_poker_draw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->add_to_hand(target_card);
        if (--target->m_game->top_request().get<request_type::poker_draw>().num_cards == 0
            || target->m_game->m_selection.size() == 0) {
            for (auto *c : target->m_game->m_selection) {
                target->m_game->move_to(c, card_pile_type::discard_pile);
            }
            target->m_game->pop_request(request_type::poker_draw);
        }
    }

    game_formatted_string request_poker_draw::status_text() const {
        return {"STATUS_POKER_DRAW", origin_card};
    }

    void request_saved::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (pile == card_pile_type::main_deck) {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request(request_type::saved);
        } else if (pile == card_pile_type::player_hand && target_player == saved) {
            for (int i=0; i<2 && !saved->m_hand.empty(); ++i) {
                target->steal_card(saved, saved->random_hand_card());
            }
            target->m_game->pop_request(request_type::saved);
        }
    }

    game_formatted_string request_saved::status_text() const {
        return {"STATUS_SAVED", origin_card, saved};
    }

    void request_add_cube::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player != target) return;
        if (pile == card_pile_type::player_character) target_card = target->m_characters.front();
        else if (pile == card_pile_type::player_table && target_card->color != card_color_type::orange) return;
        if (target_card->cubes.size() >= 4) return;
        
        target->add_cubes(target_card, 1);
        if (--target->m_game->top_request().get<request_type::add_cube>().ncubes == 0 || !target->can_receive_cubes()) {
            target->m_game->pop_request(request_type::add_cube);
        }
    }

    game_formatted_string request_add_cube::status_text() const {
        if (origin_card) {
            return {"STATUS_ADD_CUBE_FOR", origin_card};
        } else {
            return "STATUS_ADD_CUBE";
        }
    }

    void request_move_bomb::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (!target_player->immune_to(origin_card)) {
            if (target_player == target) {
                target->m_game->pop_request(request_type::move_bomb);
            } else if (!target_player->find_equipped_card(origin_card)) {
                origin_card->on_unequip(target);
                target_player->equip_card(origin_card);
                target->m_game->pop_request(request_type::move_bomb);
            }
        } else {
            target->discard_card(origin_card);
            target->m_game->pop_request(request_type::move_bomb);
        }
    }

    game_formatted_string request_move_bomb::status_text() const {
        return {"STATUS_MOVE_BOMB", origin_card};
    }

    void request_rust::on_resolve() {
        target->m_game->pop_request(request_type::rust);

        effect_rust{}.on_play(origin_card, origin, target);
    }

    game_formatted_string request_rust::status_text() const {
        return {"STATUS_RUST", origin_card};
    }

    game_formatted_string request_shopchoice::status_text() const {
        return {"STATUS_SHOPCHOICE", origin_card};
    }

    game_formatted_string timer_lemonade_jim::status_text() const {
        return {"STATUS_CAN_PLAY_CARD", origin_card};
    }

    game_formatted_string timer_al_preacher::status_text() const {
        return {"STATUS_CAN_PLAY_CARD", origin_card};
    }

    game_formatted_string timer_tumbleweed::status_text() const {
        return {"STATUS_CAN_PLAY_TUMBLEWEED", target->m_game->m_current_check->origin, origin_card, target_card, drawn_card};
    }
}