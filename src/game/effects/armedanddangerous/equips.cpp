#include "equips.h"
#include "requests.h"

#include "../../game.h"

namespace banggame {

    void effect_bomb::on_equip(card *target_card, player *target) {
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
        target->m_game->add_event<event_type::post_discard_orange_card>(target_card, [=](player *p, card *c) {
            if (c == target_card && p == target) {
                target->damage(target_card, nullptr, 2);
            }
        });
    }

    void effect_bomb::on_unequip(card *target_card, player *target) {
        predraw_check_effect{}.on_unequip(target_card, target);
        event_based_effect{}.on_unequip(target_card, target);
    }
}