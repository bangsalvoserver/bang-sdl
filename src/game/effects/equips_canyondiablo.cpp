#include "equips_canyondiablo.h"

#include "../game.h"

#include <memory>

namespace banggame {

    void effect_packmule::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::apply_maxcards_adder>(target_card, [p](player *origin, int &value) {
            if (origin == p) {
                ++value;
            }
        });
    }

    void effect_indianguide::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::apply_immunity_modifier>(target_card, [p](card *origin_card, player *origin, bool &value) {
            value = value || (origin == p && !origin_card->effects.empty() && origin_card->effects.front().is(effect_type::indians));
        });
    }

    void effect_taxman::on_equip(card *target_card, player *target) {
        target->add_predraw_check(target_card, -1, [=](card *drawn_card) {
            auto suit = target->get_card_suit(drawn_card);
            if (suit == card_suit_type::clubs || suit == card_suit_type::spades) {
                --target->m_num_cards_to_draw;
                target->m_game->add_event<event_type::post_draw_cards>(target_card, [=](player *origin) {
                    if (origin == target) {
                        ++target->m_num_cards_to_draw;
                        target->m_game->remove_events(target_card);
                    }
                });
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_lastwill::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death_priority>(target_card, [=](player *origin, player *target) {
            if (p == target) {
                target->m_game->pop_request_noupdate(request_type::death);
                target->m_game->add_request<request_type::lastwill>(target_card, target);
            }
        });
    }

    void effect_brothel::on_equip(card *target_card, player *target) {
        target->add_predraw_check(target_card, -2, [=](card *drawn_card) {
            target->discard_card(target_card);
            auto suit = target->get_card_suit(drawn_card);
            if (suit == card_suit_type::clubs || suit == card_suit_type::spades) {
                card *event_holder = new card;
                target->m_game->add_disabler(event_holder,
                    util::nocopy_wrapper([=, event_holder = std::unique_ptr<card>(event_holder)](card *c) {
                        return c->pile == card_pile_type::player_character && c->owner == target;
                    }));
                target->m_game->add_event<event_type::pre_turn_start>(event_holder, [=](player *p) {
                    if (p == target) {
                        target->m_game->remove_disablers(event_holder);
                        target->m_game->remove_events(event_holder);
                    }
                });
                target->m_game->add_event<event_type::on_player_death>(event_holder, [=](player *killer, player *p) {
                    if (p == target) {
                        target->m_game->remove_disablers(event_holder);
                        target->m_game->remove_events(event_holder);
                    }
                });
            }
            target->next_predraw_check(target_card);
        });
    }
}