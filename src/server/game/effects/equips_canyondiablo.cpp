#include "common/effects/equips_canyondiablo.h"

#include "../game.h"

namespace banggame {

    void effect_packmule::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::apply_maxcards_modifier>(target_card, [p](player *origin, int &value) {
            if (origin == p) {
                ++value;
            }
        });
    }

    void effect_indianguide::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::apply_indianguide_modifier>(target_card, [p](player *origin, bool &value) {
            if (origin == p) {
                value = true;
            }
        });
    }

    void effect_taxman::on_equip(player *target, card *target_card) {
        target->add_predraw_check(target_card, -1, [=](card *drawn_card) {
            auto suit = target->get_card_suit(drawn_card);
            if (suit == card_suit_type::clubs || suit == card_suit_type::spades) {
                target->m_game->add_event<event_type::on_draw_from_deck_priority>(target_card, [=](player *origin) {
                    if (target == origin) {
                        if (target->m_num_cards_to_draw > 0) --target->m_num_cards_to_draw;
                        target->m_game->remove_events(target_card);
                    }
                });
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_lastwill::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_player_death_priority>(target_card, [=](player *origin, player *target) {
            if (p == target) {
                target->m_game->queue_request<request_type::lastwill>(target_card, target);
            }
        });
    }
}