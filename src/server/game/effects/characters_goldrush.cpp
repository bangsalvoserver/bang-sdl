#include "common/effects/effects_goldrush.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_don_bell::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::post_turn_end>(target_card, [=](player *target) {
            if (p == target) {
                p->m_game->queue_delayed_action([target, target_card] {
                    target->m_game->draw_check_then(target, target_card, [target](card *drawn_card) {
                        card_suit_type suit = target->get_card_suit(drawn_card);
                        if (suit == card_suit_type::diamonds || suit == card_suit_type::hearts) {
                            ++target->m_extra_turns;
                        }
                    });
                });
            }
        });
    }

    void effect_madam_yto::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_play_beer>(target_card, [p](player *target) {
            p->m_game->draw_card_to(card_pile_type::player_hand, p);
        });
    }

    void effect_dutch_will::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_from_deck>(target_card, [=](player *origin) {
            if (origin == target) {
                if (target->m_num_cards_to_draw > 1) {
                    target->m_game->pop_request_noupdate(request_type::draw);
                    for (int i=0; i<target->m_num_cards_to_draw; ++i) {
                        target->m_game->draw_phase_one_card_to(card_pile_type::selection, target);
                    }
                    target->m_game->queue_request<request_type::dutch_will>(target_card, target);
                }
            }
        });
    }

    void request_dutch_will::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        ++target->m_num_drawn_cards;
        target->add_to_hand(target_card);
        target->m_game->queue_event<event_type::on_card_drawn>(target, target_card);
        if (target->m_game->m_selection.size() == 1) {
            target->m_game->pop_request(request_type::dutch_will);
            target->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::discard_pile);
            target->add_gold(1);
        }
    }

    game_formatted_string request_dutch_will::status_text() const {
        return {"STATUS_DUTCH_WILL", origin_card};
    }

    void effect_josh_mccloud::on_play(card *origin_card, player *target) {
        auto *card = target->m_game->draw_shop_card();
        if (!target->is_possible_to_play(card)) {
            target->m_game->move_to(card, card_pile_type::shop_discard, true, nullptr, show_card_flags::pause_before_move);
        } else {
            target->set_forced_card(card);
        }
    }
}