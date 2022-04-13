#include "characters_base.h"
#include "requests_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_calamity_janet::on_equip(card *origin_card, player *p) {
        p->add_player_flags(player_flags::treat_missed_as_bang);
    }

    void effect_calamity_janet::on_unequip(card *origin_card, player *p) {
        p->remove_player_flags(player_flags::treat_missed_as_bang);
    }

    void effect_slab_the_killer::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::apply_bang_modifier>(target_card, [p](player *target, request_bang *req) {
            if (p == target) {
                ++req->bang_strength;
            }
        });
    }

    void effect_black_jack::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_card_drawn>(target_card, [target](player *origin, card *drawn_card) {
            if (origin == target && origin->m_num_drawn_cards == 2) {
                card_suit suit = target->get_card_sign(drawn_card).suit;
                target->m_game->send_card_update(*drawn_card, nullptr, show_card_flags::short_pause);
                target->m_game->send_card_update(*drawn_card, target);
                if (suit == card_suit::hearts || suit == card_suit::diamonds) {
                    origin->m_game->queue_action([=]{
                        ++origin->m_num_drawn_cards;
                        card *drawn_card = origin->m_game->draw_phase_one_card_to(pocket_type::player_hand, origin);
                        origin->m_game->call_event<event_type::on_card_drawn>(target, drawn_card);
                    });
                }
            }
        });
    }

    void effect_kit_carlson::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_from_deck>(target_card, [=](player *origin) {
            if (target == origin && target->m_num_cards_to_draw < 3) {
                target->m_game->pop_request_noupdate<request_draw>();
                for (int i=0; i<3; ++i) {
                    target->m_game->draw_phase_one_card_to(pocket_type::selection, target);
                }
                target->m_game->queue_request<request_kit_carlson>(target_card, target);
            }
        });
    }

    void request_kit_carlson::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        ++target->m_num_drawn_cards;
        target->add_to_hand(target_card);
        target->m_game->call_event<event_type::on_card_drawn>(target, target_card);
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            while (!target->m_game->m_selection.empty()) {
                target->m_game->move_to(target->m_game->m_selection.front(), pocket_type::main_deck, false);
            }
            target->m_game->pop_request<request_kit_carlson>();
        }
    }

    game_formatted_string request_kit_carlson::status_text(player *owner) const {
        if (owner == target) {
            return {"STATUS_KIT_CARLSON", origin_card};
        } else {
            return {"STATUS_KIT_CARLSON_OTHER", target, origin_card};
        }
    }

    void effect_el_gringo::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 2}, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && p == target && p->m_game->m_playing != p) {
                target->m_game->queue_action([=]{
                    if (target->alive() && !origin->m_hand.empty()) {
                        target->steal_card(origin, origin->random_hand_card());
                        target->m_game->call_event<event_type::on_effect_end>(p, target_card);
                    }
                });
            }
        });
    }

    void effect_suzy_lafayette::on_equip(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::on_effect_end>(origin_card, [origin](player *, card *) {
            origin->m_game->queue_action([origin]{
                if (origin->alive() && origin->m_hand.empty()) {
                    origin->m_game->draw_card_to(pocket_type::player_hand, origin);
                }
            });
        });
    }

    void effect_vulture_sam::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                for (auto it = target->m_table.begin(); it != target->m_table.end(); ) {
                    card *target_card = *it;
                    if (target_card->color != card_color_type::black) {
                        it = target->move_card_to(target_card, pocket_type::player_hand, true, p);
                    } else {
                        ++it;
                    }
                }
                while (!target->m_hand.empty()) {
                    p->add_to_hand(target->m_hand.front());
                }
            }
        });
    }

}