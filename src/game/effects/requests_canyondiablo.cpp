#include "requests_canyondiablo.h"

#include "../game.h"

namespace banggame {

    void request_card_sharper::on_resolve() {
        target->m_game->pop_request(request_type::card_sharper);
        
        card_sharper_handler{origin_card, origin, target, chosen_card, target_card}.on_resolve();
    }

    game_formatted_string request_card_sharper::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_CARD_SHARPER", origin_card, target_card, chosen_card};
        } else {
            return {"STATUS_CARD_SHARPER_OTHER", target, origin_card, target_card, chosen_card};
        }
    }

    bool request_lastwill::can_pick(card_pile_type pile, player *p, card *target_card) const {
        return (pile == card_pile_type::player_hand || pile == card_pile_type::player_table)
            && p == target && target_card != origin_card;
    }

    void request_lastwill::on_pick(card_pile_type pile, player *p, card *target_card) {
        target->m_game->move_to(target_card, card_pile_type::selection, true, target);
        if (--target->m_game->top_request().get<request_type::lastwill>().ncards == 0) {
            on_resolve();
        }
    }

    void request_lastwill::on_resolve() {
        if (target->m_game->m_selection.empty()) {
            target->m_game->player_death(origin, target);
            target->m_game->pop_request(request_type::lastwill);
            target->m_game->check_game_over(origin, target);
        } else {
            target->m_game->pop_request_noupdate(request_type::lastwill);
            target->m_game->add_request<request_type::lastwill_target>(origin_card, target);
        }
    }

    game_formatted_string request_lastwill::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_LASTWILL", origin_card};
        } else {
            return {"STATUS_LASTWILL_OTHER", target, origin_card};
        }
    }

    bool request_lastwill_target::can_pick(card_pile_type pile, player *p, card *) const {
        return pile == card_pile_type::player && p != target && p->alive();
    }

    void request_lastwill_target::on_pick(card_pile_type pile, player *p, card *) {
        if (p != target) {
            while (!target->m_game->m_selection.empty()) {
                target->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::player_hand, true, p);
            }
            
            target->m_game->player_death(origin, target);
            target->m_game->pop_request(request_type::lastwill_target);
            target->m_game->check_game_over(origin, target);
        }
    }

    game_formatted_string request_lastwill_target::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_LASTWILL_TARGET", origin_card};
        } else {
            return {"STATUS_LASTWILL_TARGET_OTHER", target, origin_card};
        }
    }
}