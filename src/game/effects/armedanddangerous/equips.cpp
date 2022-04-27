#include "equips.h"
#include "requests.h"

#include "../../game.h"

namespace banggame {

    void effect_bomb::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_discard_orange_card>(target_card, [=](player *e_target, card *e_card) {
            if (e_target == target && e_card == target_card
                && !target->m_game->is_disabled(target_card) && !target->immune_to(target_card)) {
                target->m_game->add_log("LOG_CARD_EXPLODES", target_card);
                target->damage(target_card, nullptr, 2);
            }
        });
    }

    void effect_bomb::on_enable(card *target_card, player *target) {
        target->add_predraw_check(target_card, 0, [=](card *drawn_card) {
            card_suit suit = target->get_card_sign(drawn_card).suit;
            if (suit == card_suit::spades || suit == card_suit::clubs) {
                target->pay_cubes(target_card, 2);
            } else {
                target->m_game->queue_request_front<request_move_bomb>(target_card, target);
                target->set_forced_card(target_card);
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_bomb::on_unequip(card *target_card, player *target) {
        target->m_game->remove_events(target_card);
    }
}