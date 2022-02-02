#include "common/effects/effects_goldrush.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_don_bell::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_turn_end>(target_card, [=](player *target) {
            if (p == target) {
                if (target_card->max_usages == 0) {
                    p->m_game->m_ignore_next_turn = true;
                    p->m_game->draw_check_then(p, target_card, [&](card *drawn_card) {
                        card_suit_type suit = p->get_card_suit(drawn_card);
                        if (suit == card_suit_type::diamonds || suit == card_suit_type::hearts) {
                            ++target_card->max_usages;
                            p->start_of_turn();
                        } else {
                            p->m_game->get_next_in_turn(p)->start_of_turn();
                        }
                    });
                } else {
                    target_card->max_usages = 0;
                }
            }
        });
    }

    void effect_madam_yto::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_play_beer>(target_card, [p](player *target) {
            p->m_game->draw_card_to(card_pile_type::player_hand, p);
        });
    }

    void effect_dutch_will::on_equip(player *target, card *target_card) {
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
        using namespace enums::flag_operators;

        constexpr auto is_self_or_none = [](target_type type) {
            using namespace enums::flag_operators;
            return type == (target_type::self | target_type::player) || type == enums::flags_none<target_type>;
        };

        auto *card = target->m_game->draw_shop_card();
        switch (card->color) {
        case card_color_type::black:
            if (target->m_game->has_scenario(scenario_flags::judge)) {
                target->m_game->move_to(card, card_pile_type::shop_discard);
            } else if (std::ranges::all_of(card->equips, is_self_or_none, &equip_holder::target)) {
                target->equip_card(card);
            } else {
                target->m_game->queue_request<request_type::shop_choose_target>(card, target);
            }
            break;
        case card_color_type::brown:
            if (std::ranges::all_of(card->effects, is_self_or_none, &effect_holder::target)) {
                target->m_game->move_to(card, card_pile_type::shop_discard);
                for (auto &e : card->effects) {
                    switch(e.target) {
                    case target_type::self | target_type::player:
                        e.on_play(card, target, target);
                        break;
                    case enums::flags_none<target_type>:
                        e.on_play(card, target);
                        break;
                    }
                }
            } else {
                target->m_game->queue_request<request_type::shop_choose_target>(card, target);
            }
            break;
        }
    }

    void request_shop_choose_target::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_game->pop_request(request_type::shop_choose_target);
        if (origin_card->color == card_color_type::black) {
            target_player->equip_card(origin_card);
        } else {
            target->m_game->move_to(origin_card, card_pile_type::shop_discard);
            for (auto &e : origin_card->effects) {
                e.on_play(origin_card, target, target_player);
            }
        }
    }

    game_formatted_string request_shop_choose_target::status_text() const {
        return {"STATUS_SHOP_CHOOSE_TARGET", origin_card};
    }
}