#include "requests.h"

#include "../valleyofshadows/requests.h"
#include "../armedanddangerous/requests.h"

#include "../../game.h"

namespace banggame {

    bool request_characterchoice::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target;
    }

    void request_characterchoice::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_log("LOG_CHARACTER_CHOICE", target, target_card);
        target->m_game->move_card(target_card, pocket_type::player_character, target, show_card_flags::shown);
        target_card->on_enable(target);

        target->m_hp = target->m_max_hp;
        target->m_game->add_update<game_update_type::player_hp>(target->id, target->m_hp, false, true);

        target->m_game->move_card(target->m_hand.front(), pocket_type::player_backup, target, show_card_flags::hidden);
        target->m_game->pop_request<request_characterchoice>();
    }

    game_formatted_string request_characterchoice::status_text(player *owner) const {
        if (owner == target) {
            return "STATUS_CHARACTERCHOICE";
        } else {
            return {"STATUS_CHARACTERCHOICE_OTHER", target};
        }
    }

    bool request_draw::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return target->m_game->has_scenario(scenario_flags::abandonedmine) && !target->m_game->m_discards.empty()
            ? pocket == pocket_type::discard_pile
            : pocket == pocket_type::main_deck;
    }

    void request_draw::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->draw_from_deck();
    }

    game_formatted_string request_draw::status_text(player *owner) const {
        if (owner == target) {
            return "STATUS_YOUR_TURN";
        } else {
            return {"STATUS_YOUR_TURN_OTHER", target};
        }
    }
    
    bool request_predraw::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        if (pocket == pocket_type::player_table && target == target_player) {
            int top_priority = std::ranges::max(target->m_predraw_checks
                | std::views::values
                | std::views::filter(std::not_fn(&player::predraw_check::resolved))
                | std::views::transform(&player::predraw_check::priority));
            auto it = target->m_predraw_checks.find(target_card);
            if (it != target->m_predraw_checks.end()
                && !it->second.resolved
                && it->second.priority == top_priority) {
                return true;
            }
        }
        return false;
    }
    
    void request_predraw::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->pop_request<request_predraw>();
        target->m_game->draw_check_then(target, target_card, target->m_predraw_checks.find(target_card)->second.check_fun);
    }

    game_formatted_string request_predraw::status_text(player *owner) const {
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
            if (owner == target) {
                return {"STATUS_PREDRAW_FOR", top_priority.begin()->first};
            } else {
                return {"STATUS_PREDRAW_FOR_OTHER", target, top_priority.begin()->first};
            }
        } else if (owner == target) {
            return "STATUS_PREDRAW";
        } else {
            return {"STATUS_PREDRAW_OTHER", target};
        }
    }

    void request_check::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->pop_request_noupdate<request_check>();
        target->m_game->m_current_check.select(target_card);
        target->m_game->update_request();
    }

    game_formatted_string request_check::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_CHECK", origin_card};
        } else {
            return {"STATUS_CHECK_OTHER", target, origin_card};
        }
    }

    void request_generalstore::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        auto next = target->m_game->get_next_player(target);
        if (target->m_game->m_selection.size() == 2) {
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card, origin_card);
            target->add_to_hand(target_card);
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", next, target->m_game->m_selection.front(), origin_card);
            next->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request<request_generalstore>();
        } else {
            target->m_game->pop_request_noupdate<request_generalstore>();
            target->m_game->add_log("LOG_DRAWN_FROM_GENERALSTORE", target, target_card, origin_card);
            target->add_to_hand(target_card);
            target->m_game->queue_request<request_generalstore>(origin_card, origin, next);
        }
    }

    game_formatted_string request_generalstore::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_GENERALSTORE", origin_card};
        } else {
            return {"STATUS_GENERALSTORE_OTHER", target, origin_card};
        }
    }

    bool request_discard::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target;
    }
    
    void request_discard::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_log("LOG_DISCARDED_CARD_FOR", origin_card, target, target_card);
        target->discard_card(target_card);
        target->m_game->call_event<event_type::on_effect_end>(target, origin_card);
        
        if (--ncards == 0) {
            target->m_game->pop_request<request_discard>();
        }
    }

    game_formatted_string request_discard::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_DISCARD", origin_card};
        } else {
            return {"STATUS_DISCARD_OTHER", target, origin_card};
        }
    }

    bool request_discard_pass::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target;
    }

    void request_discard_pass::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, target_card);
        if (target->m_game->has_scenario(scenario_flags::abandonedmine)) {
            target->m_game->move_card(target_card, pocket_type::main_deck, nullptr, show_card_flags::hidden);
        } else {
            target->discard_card(target_card);
        }
        ++ndiscarded;
        target->m_game->call_event<event_type::on_discard_pass>(target, target_card);
        if (target->m_hand.size() <= target->max_cards_end_of_turn()) {
            if (target->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                target->queue_request_add_cube(nullptr, ndiscarded);
            }
            target->m_game->pop_request<request_discard_pass>();
            target->m_game->queue_action([target = target]{ target->pass_turn(); });
        } else {
            target->m_game->update_request();
        }
    }

    game_formatted_string request_discard_pass::status_text(player *owner) const {
        int diff = target->m_hand.size() - target->max_cards_end_of_turn();
        if (diff > 1) {
            if (target == owner) {
                return {"STATUS_DISCARD_PASS_PLURAL", diff};
            } else {
                return {"STATUS_DISCARD_PASS_PLURAL_OTHER", target, diff};
            }
        } else if (target == owner) {
            return "STATUS_DISCARD_PASS";
        } else {
            return {"STATUS_DISCARD_PASS_OTHER", target};
        }
    }

    bool request_indians::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target && target_card->equips.empty() && target->is_bangcard(target_card);
    }

    void request_indians::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_log("LOG_RESPONDED_WITH_CARD", target_card, target);
        target->m_game->call_event<event_type::on_play_hand_card>(target, target_card);
        target->discard_card(target_card);
        target->m_game->pop_request<request_indians>();
    }

    void request_indians::on_resolve() {
        target->m_game->pop_request_noupdate<request_indians>();
        target->damage(origin_card, origin, 1);
        target->m_game->update_request();
    }

    game_formatted_string request_indians::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_INDIANS", origin_card};
        } else {
            return {"STATUS_INDIANS_OTHER", target, origin_card};
        }
    }

    bool request_duel::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target && target_card->equips.empty() && target->is_bangcard(target_card);
    }

    void request_duel::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_log("LOG_RESPONDED_WITH_CARD", target_card, target);
        target->m_game->call_event<event_type::on_play_hand_card>(target, target_card);
        target->discard_card(target_card);
        target->m_game->pop_request_noupdate<request_duel>();
        target->m_game->queue_request<request_duel>(origin_card, origin, respond_to, target);
    }

    void request_duel::on_resolve() {
        target->m_game->pop_request_noupdate<request_duel>();
        target->damage(origin_card, origin, 1);
        target->m_game->update_request();
    }

    game_formatted_string request_duel::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_DUEL", origin_card};
        } else {
            return {"STATUS_DUEL_OTHER", target, origin_card};
        }
    }

    void missable_request::on_miss(player *origin) {
        origin->m_game->pop_request<missable_request>();
    }

    bool request_bang::can_respond(card *c) const {
        return !unavoidable && missable_request::can_respond(c);
    }

    void request_bang::on_miss(player *p) {
        if (--bang_strength == 0) {
            p->m_game->call_event<event_type::on_missed>(origin_card, origin, target, is_bang_card);
            p->m_game->pop_request<request_bang>();
        } else {
            p->m_game->update_request();
        }
    }

    void request_bang::on_resolve() {
        target->m_game->pop_request_noupdate<request_bang>();
        target->damage(origin_card, origin, bang_damage, is_bang_card);
        if (auto *req = target->m_game->top_request_if<timer_damaging>(target)) {
            static_cast<cleanup_request &>(*req) = std::move(*this);
        } else {
            target->m_game->update_request();
        }
    }

    game_formatted_string request_bang::status_text(player *owner) const {
        if (target != owner) {
            return {"STATUS_BANG_OTHER", target, origin_card};
        } else if (unavoidable) {
            return {"STATUS_BANG_UNAVOIDABLE", origin_card};
        } else if (bang_strength > 1) {
            return {"STATUS_BANG_MULTIPLE_MISSED", origin_card, bang_strength};
        } else {
            return {"STATUS_BANG", origin_card};
        }
    }

    void request_death::on_resolve() {
        target->m_game->player_death(origin, target);
        target->m_game->pop_request_noupdate<request_death>();
        target->m_game->queue_action_front([origin=origin, target=target]{
            target->m_game->check_game_over(origin, target);
        });
        target->m_game->update_request();
    }

    game_formatted_string request_death::status_text(player *owner) const {
        if (target == owner) {
            return "STATUS_DEATH";
        } else {
            return {"STATUS_DEATH_OTHER", target};
        }
    }
}