#include "equips.h"

#include "../../game.h"

namespace banggame {

    void effect_luckycharm::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>(target_card, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (p == target) {
                target->m_game->queue_action([=]{
                    if (target->alive()) {
                        target->add_gold(damage);
                    }
                });
            }
        });
    }

    void effect_pickaxe::on_enable(card *target_card, player *target) {
        ++target->m_num_cards_to_draw;
    }

    void effect_pickaxe::on_disable(card *target_card, player *target) {
        --target->m_num_cards_to_draw;
    }

    void effect_calumet::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_immunity_modifier>(target_card, [=](card *origin_card, player *p, bool &value) {
            value = value || (p == target && p->get_card_sign(origin_card).suit == card_suit::diamonds);
        });
    }

    void effect_gunbelt::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_maxcards_modifier>({target_card, 20 - ncards}, [=, ncards=ncards](player *p, int &value) {
            if (p == target) {
                value = ncards;
            }
        });
    }

    void effect_wanted::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p, target_card](player *origin, player *target) {
            if (origin && p == target && origin != target) {
                origin->draw_card(2, target_card);
                origin->add_gold(1);
            }
        });
    }

}