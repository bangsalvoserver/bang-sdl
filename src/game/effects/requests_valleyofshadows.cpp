#include "requests_valleyofshadows.h"
#include "effects_valleyofshadows.h"
#include "effects_base.h"

#include "../game.h"

namespace banggame {
    
    void timer_damaging::on_finished() {
        if (target->m_hp <= damage) {
            target->m_game->pop_request_noupdate<timer_damaging>();
        } else {
            target->m_game->pop_request<timer_damaging>();
        }
        target->damage(origin_card, origin, damage, is_bang, true);
    }

    game_formatted_string timer_damaging::status_text(player *owner) const {
        return {damage > 1 ? "STATUS_DAMAGING_PLURAL" : "STATUS_DAMAGING", target, origin_card, damage};
    }

    void request_destroy::on_resolve() {
        effect_destroy{}.on_resolve(origin_card, origin, target, target_card);
        target->m_game->pop_request<request_destroy>();
    }

    game_formatted_string request_destroy::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_DESTROY", origin_card, with_owner{target_card}};
        } else {
            return {"STATUS_DESTROY_OTHER", target, origin_card, with_owner{target_card}};
        }
    }

    void request_steal::on_resolve() {
        effect_steal{}.on_resolve(origin_card, origin, target, target_card);
        target->m_game->pop_request<request_steal>();
    }

    game_formatted_string request_steal::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_STEAL", origin_card, with_owner{target_card}};
        } else {
            return {"STATUS_STEAL_OTHER", target, origin_card, with_owner{target_card}};
        }
    }

    bool request_bandidos::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_bandidos::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->discard_card(target_card);
        if (--num_cards == 0 || target->m_hand.empty()) {
            target->m_game->pop_request<request_bandidos>();
        }
    }

    void request_bandidos::on_resolve() {
        target->m_game->pop_request_noupdate<request_bandidos>();
        target->damage(origin_card, origin, 1);
        target->m_game->update_request();
    }

    game_formatted_string request_bandidos::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_BANDIDOS", origin_card};
        } else {
            return {"STATUS_BANDIDOS_OTHER", target, origin_card};
        }
    }

    bool request_tornado::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_tornado::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->discard_card(target_card);
        target->m_game->draw_card_to(card_pile_type::player_hand, target);
        target->m_game->draw_card_to(card_pile_type::player_hand, target);
        target->m_game->pop_request<request_tornado>();
    }

    game_formatted_string request_tornado::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_TORNADO", origin_card};
        } else {
            return {"STATUS_TORNADO_OTHER", target, origin_card};
        }
    }

    bool request_poker::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_poker::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_game->move_to(target_card, card_pile_type::selection, true, origin);
        target->m_game->pop_request<request_poker>();
    }

    game_formatted_string request_poker::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_POKER", origin_card};
        } else {
            return {"STATUS_POKER_OTHER", target, origin_card};
        }
    }

    void request_poker_draw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->add_to_hand(target_card);
        if (--num_cards == 0 || target->m_game->m_selection.size() == 0) {
            for (auto *c : target->m_game->m_selection) {
                target->m_game->move_to(c, card_pile_type::discard_pile);
            }
            target->m_game->pop_request<request_poker_draw>();
        }
    }

    game_formatted_string request_poker_draw::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_POKER_DRAW", origin_card};
        } else {
            return {"STATUS_POKER_DRAW_OTHER", target, origin_card};
        }
    }

    bool request_saved::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::main_deck
            || (pile == card_pile_type::player_hand && target_player == saved);
    }

    void request_saved::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        switch (pile) {
        case card_pile_type::main_deck:
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request<request_saved>();
            break;
        case card_pile_type::player_hand:
            for (int i=0; i<2 && !saved->m_hand.empty(); ++i) {
                target->steal_card(saved, saved->random_hand_card());
            }
            target->m_game->pop_request<request_saved>();
            break;
        }
    }

    game_formatted_string request_saved::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_SAVED", origin_card, saved};
        } else {
            return {"STATUS_SAVED_OTHER", target, origin_card, saved};
        }
    }

    game_formatted_string timer_lemonade_jim::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_CAN_PLAY_CARD", origin_card};
        } else {
            return {"STATUS_CAN_PLAY_CARD_OTHER", target, origin_card};
        }
    }
}