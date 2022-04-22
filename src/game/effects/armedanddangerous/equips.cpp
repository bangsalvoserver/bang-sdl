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
    }

    void effect_bomb::on_post_unequip(card *target_card, player *target) {
        if (target_card->cubes.empty() && !target->immune_to(target_card)) {
            target->damage(target_card, nullptr, 2);
        }
    }
}