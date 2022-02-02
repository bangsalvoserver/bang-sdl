#include "common/effects/characters_valleyofshadows.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_tuco_franziskaner::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_draw_from_deck>(target_card, [p](player *origin) {
            if (p == origin && std::ranges::find(p->m_table, card_color_type::blue, &card::color) == p->m_table.end()) {
                p->m_num_cards_to_draw += 2;
            }
        });
    }

    void effect_colorado_bill::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_play_bang>(target_card, [=](player *origin) {
            if (p == origin) {
                origin->m_game->draw_check_then(origin, target_card, [=](card *drawn_card) {
                    if (p->get_card_suit(drawn_card) == card_suit_type::spades) {
                        origin->add_bang_mod([](request_bang &req) {
                            req.unavoidable = true;
                        });
                    }
                });
            }
        });
    }

    void effect_henry_block::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_discard_card>(target_card, [=](player *origin, player *target, card *discarded_card) {
            if (p == target && p != origin) {
                p->m_game->queue_request<request_type::bang>(target_card, target, origin);
            }
        });
    }
    
    void effect_lemonade_jim::on_equip(player *origin, card *target_card) {
        origin->m_game->add_event<event_type::on_play_beer>(target_card, [=](player *target) {
            if (origin != target) {
                if (!origin->m_hand.empty() && origin->m_hp < origin->m_max_hp) {
                    target->m_game->queue_request<request_type::lemonade_jim>(target_card, target, origin);
                }
            }
        });
    }

    bool effect_lemonade_jim::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::lemonade_jim, origin);
    }

    void effect_lemonade_jim::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request(request_type::lemonade_jim);
    }
}