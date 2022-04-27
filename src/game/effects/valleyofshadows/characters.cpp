#include "characters.h"
#include "requests.h"

#include "../base/effects.h"
#include "../base/requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_tuco_franziskaner::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_draw_from_deck>(target_card, [p](player *origin) {
            if (p == origin && std::ranges::find(p->m_table, card_color_type::blue, &card::color) == p->m_table.end()) {
                p->m_num_cards_to_draw += 2;
            }
        });
    }

    void effect_colorado_bill::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::apply_bang_modifier>(target_card, [=](player *origin, request_bang *req) {
            if (p == origin) {
                origin->m_game->draw_check_then(origin, target_card, [=](card *drawn_card) {
                    if (p->get_card_sign(drawn_card).suit == card_suit::spades) {
                        p->m_game->add_log("LOG_CARD_HAS_EFFECT", target_card);
                        req->unavoidable = true;
                    }
                });
            }
        });
    }

    void effect_henry_block::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_discard_card>(target_card, [=](player *origin, player *target, card *discarded_card) {
            if (p == target && p != origin) {
                p->m_game->add_log("LOG_CARD_HAS_EFFECT", target_card);
                p->m_game->queue_request<request_bang>(target_card, target, origin);
            }
        });
    }
    
    void effect_lemonade_jim::on_enable(card *target_card, player *origin) {
        origin->m_game->add_event<event_type::on_play_beer>(target_card, [=](player *target) {
            if (origin != target) {
                if (!origin->m_hand.empty() && origin->m_hp < origin->m_max_hp) {
                    target->m_game->queue_request<timer_lemonade_jim>(target_card, target, origin);
                }
            }
        });
    }

    bool effect_lemonade_jim::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<timer_lemonade_jim>(origin);
    }

    void effect_lemonade_jim::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request<timer_lemonade_jim>();
    }

    opt_error effect_evelyn_shebang::verify(card *origin_card, player *origin, player *target) const {
        bool valid = true;
        origin->m_game->call_event<event_type::verify_target_unique>(origin_card, origin, target, valid);
        if (!valid) {
            return game_error("ERROR_TARGET_NOT_UNIQUE");
        }
        return std::nullopt;
    }

    void effect_evelyn_shebang::on_play(card *origin_card, player *origin, player *target) {
        origin->m_game->add_event<event_type::verify_target_unique>(origin_card, [=](card *e_origin_card, player *e_origin, player *e_target, bool &value) {
            if (e_origin_card == origin_card && e_origin == origin && e_target == target) {
                value = false;
            }
        });

        origin->m_game->add_event<event_type::on_turn_end>(origin_card, [=](player *e_origin) {
            if (origin == e_origin) {
                origin->m_game->remove_events(origin_card);
            }
        });

        origin->m_game->pop_request_noupdate<request_draw>();
        ++origin->m_num_drawn_cards;

        effect_bang().on_play(origin_card, origin, target);

        origin->m_game->queue_action([=]{
            if (origin->m_num_drawn_cards < origin->m_num_cards_to_draw && origin->m_game->m_playing == origin) {
                origin->request_drawing();
            }
        });
    }
}