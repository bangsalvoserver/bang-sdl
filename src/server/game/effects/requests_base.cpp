#include "common/effects/requests_base.h"

#include "../game.h"

namespace banggame {

    bool request_characterchoice::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_characterchoice::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_game->move_to(target_card, card_pile_type::player_character, true, target, show_card_flags::show_everyone);
        target->equip_if_enabled(target_card);

        target->m_hp = target->m_max_hp = static_cast<character *>(target_card)->max_hp + (target->m_role == player_role::sheriff);
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, target->m_hp, false, true);

        target->m_game->move_to(target->m_hand.front(), card_pile_type::player_backup, false, target);
        target->m_game->pop_request(request_type::characterchoice);
    }

    game_formatted_string request_characterchoice::status_text() const {
        return "STATUS_CHARACTERCHOICE";
    }

    bool request_draw::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return target->m_game->has_scenario(scenario_flags::abandonedmine) && !target->m_game->m_discards.empty()
            ? pile == card_pile_type::discard_pile
            : pile == card_pile_type::main_deck;
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
        target->m_game->pop_request_noupdate(request_type::check);
        target->m_game->add_log("LOG_CHECK_DREW_CARD", target->m_game->m_current_check->origin_card, target, target_card);
        target->m_game->instant_event<event_type::trigger_tumbleweed>(target->m_game->m_current_check->origin_card, target_card);
        if (!target->m_game->top_request_is(request_type::tumbleweed)) {
            target->m_game->m_current_check->function(target_card);
            target->m_game->m_current_check.reset();
            target->m_game->events_after_requests();
        }
    }

    game_formatted_string request_check::status_text() const {
        return {"STATUS_CHECK", origin_card};
    }

    void request_generalstore::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto next = target->m_game->get_next_player(target);
        if (target->m_game->m_selection.size() == 2) {
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card, origin_card);
            target->add_to_hand(target_card);
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", next, target->m_game->m_selection.front(), origin_card);
            next->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request(request_type::generalstore);
        } else {
            target->m_game->pop_request_noupdate(request_type::generalstore);
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card, origin_card);
            target->add_to_hand(target_card);
            target->m_game->queue_request<request_type::generalstore>(origin_card, origin, next);
        }
    }

    game_formatted_string request_generalstore::status_text() const {
        return {"STATUS_GENERALSTORE", origin_card};
    }

    bool request_discard::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }
    
    void request_discard::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (--target->m_game->top_request().get<request_type::discard>().ncards == 0) {
            target->m_game->pop_request(request_type::discard);
        }

        target->discard_card(target_card);
        target->m_game->queue_event<event_type::on_effect_end>(target, origin_card);
    }

    game_formatted_string request_discard::status_text() const {
        return {"STATUS_DISCARD", origin_card};
    }

    bool request_discard_pass::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_discard_pass::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target->m_game->has_scenario(scenario_flags::abandonedmine)) {
            target->move_card_to(target_card, card_pile_type::main_deck);
        } else {
            target->discard_card(target_card);
        }
        target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, target_card);
        target->m_game->instant_event<event_type::on_discard_pass>(target, target_card);
        if (target->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
            target->m_game->queue_delayed_action([target = target]{
                if (target->can_receive_cubes()) {
                    target->m_game->queue_request<request_type::add_cube>(nullptr, target);
                }
            });
        }
        if (target->m_hand.size() <= target->max_cards_end_of_turn()) {
            target->m_game->pop_request(request_type::discard_pass);
            target->m_game->queue_delayed_action(std::bind(&player::pass_turn, target));
        }
    }

    game_formatted_string request_discard_pass::status_text() const {
        int diff = target->m_hand.size() - target->max_cards_end_of_turn();
        if (diff > 1) {
            return {"STATUS_DISCARD_PASS_PLURAL", diff};
        } else {
            return "STATUS_DISCARD_PASS";
        }
    }

    bool request_indians::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target && target->is_bangcard(target_card);
    }

    void request_indians::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->discard_card(target_card);
        target->m_game->pop_request(request_type::indians);
    }

    void request_indians::on_resolve() {
        target->m_game->pop_request(request_type::indians);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_indians::status_text() const {
        return {"STATUS_INDIANS", origin_card};
    }

    bool request_duel::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target && target->is_bangcard(target_card);
    }

    void request_duel::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->discard_card(target_card);
        target->m_game->pop_request_noupdate(request_type::duel);
        target->m_game->queue_request<request_type::duel>(origin_card, origin, respond_to, target);
    }

    void request_duel::on_resolve() {
        target->m_game->pop_request(request_type::duel);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_duel::status_text() const {
        return {"STATUS_DUEL", origin_card};
    }

    void request_bang::on_resolve() {
        target->m_game->pop_request_noupdate(request_type::bang);
        target->damage(origin_card, origin, bang_damage, is_bang_card);
        if (!target->m_game->m_requests.empty() && target->m_game->m_requests.back().is(request_type::damaging)) {
            auto &req = target->m_game->m_requests.back().get<request_type::damaging>();
            req.cleanup_function = std::move(cleanup_function);
        } else {
            target->m_game->events_after_requests();
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
        target->m_game->instant_event<event_type::on_player_death_priority>(origin, target);
        if (target->m_game->top_request_is(request_type::death)) {
            target->m_game->player_death(origin, target);
            target->m_game->pop_request(request_type::death);
        }
        target->m_game->queue_delayed_action([origin = origin, target = target]{
            target->m_game->check_game_over(origin, target);
        });
    }

    game_formatted_string request_death::status_text() const {
        return "STATUS_DEATH";
    }
}