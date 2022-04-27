#include "characters.h"
#include "requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_calamity_janet::on_enable(card *origin_card, player *p) {
        p->add_player_flags(player_flags::treat_missed_as_bang);
    }

    void effect_calamity_janet::on_disable(card *origin_card, player *p) {
        p->remove_player_flags(player_flags::treat_missed_as_bang);
    }

    void effect_slab_the_killer::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::apply_bang_modifier>(target_card, [p](player *target, request_bang *req) {
            if (p == target) {
                ++req->bang_strength;
            }
        });
    }

    void effect_black_jack::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_card_drawn>(target_card, [target](player *origin, card *drawn_card) {
            if (origin == target && origin->m_num_drawn_cards == 2) {
                card_suit suit = target->get_card_sign(drawn_card).suit;
                target->m_game->add_log(update_target::excludes(target), "LOG_REVEALED_CARD", target, drawn_card);
                target->m_game->send_card_update(drawn_card, nullptr, show_card_flags::short_pause);
                target->m_game->send_card_update(drawn_card, target);
                if (suit == card_suit::hearts || suit == card_suit::diamonds) {
                    origin->m_game->queue_action([=]{
                        ++origin->m_num_drawn_cards;
                        card *drawn_card = origin->m_game->phase_one_drawn_card();
                        target->m_game->add_log(update_target::excludes(target), "LOG_DRAWN_A_CARD", target);
                        target->m_game->add_log(update_target::includes(target), "LOG_DRAWN_CARD", target, drawn_card);
                        origin->m_game->draw_phase_one_card_to(pocket_type::player_hand, origin);
                        origin->m_game->call_event<event_type::on_card_drawn>(target, drawn_card);
                    });
                }
            }
        });
    }

    void effect_kit_carlson::on_enable(card *target_card, player *target) {
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
        target->m_game->add_log(update_target::excludes(target), "LOG_DRAWN_A_CARD", target);
        target->m_game->add_log(update_target::includes(target), "LOG_DRAWN_CARD", target, target_card);
        target->add_to_hand(target_card);
        target->m_game->call_event<event_type::on_card_drawn>(target, target_card);
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            while (!target->m_game->m_selection.empty()) {
                target->m_game->move_card(target->m_game->m_selection.front(), pocket_type::main_deck, nullptr, show_card_flags::hidden);
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

    void effect_el_gringo::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 2}, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && p == target && p->m_game->m_playing != p) {
                target->m_game->queue_action([=]{
                    if (target->alive() && !origin->m_hand.empty()) {
                        card *stolen_card = origin->random_hand_card();
                        target->m_game->add_log(update_target::includes(origin, target), "LOG_STOLEN_CARD", target, origin, stolen_card);
                        target->m_game->add_log(update_target::excludes(origin, target), "LOG_STOLEN_CARD_FROM_HAND", target, origin);
                        target->steal_card(stolen_card);
                        target->m_game->call_event<event_type::on_effect_end>(p, target_card);
                    }
                });
            }
        });
    }

    void effect_suzy_lafayette::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::on_effect_end>(origin_card, [origin, origin_card](player *, card *) {
            origin->m_game->queue_action([origin, origin_card]{
                if (origin->alive() && origin->m_hand.empty()) {
                    origin->draw_card(1, origin_card);
                }
            });
        });
    }

    void effect_vulture_sam::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                std::vector<card *> target_cards;
                for (card *c : target->m_table) {
                    if (c->color != card_color_type::black) {
                        target_cards.push_back(c);
                    }
                }
                for (card *c : target->m_hand) {
                    target_cards.push_back(c);
                }

                auto next_vulture_sam = [p = target, target]() mutable {
                    while (true) {
                        p = p->m_game->get_next_player(p);
                        if (p != target && std::ranges::any_of(p->m_characters, [](card *c) {
                            return c->equips.last_is(equip_type::vulture_sam);
                        })) return p;
                    }
                };

                for (card *c : target_cards) {
                    player *vulture_sam = next_vulture_sam();
                    if (c->pocket == pocket_type::player_hand) {
                        target->m_game->add_log(update_target::includes(target, vulture_sam), "LOG_STOLEN_CARD", vulture_sam, target, c);
                        target->m_game->add_log(update_target::excludes(target, vulture_sam), "LOG_STOLEN_CARD_FROM_HAND", vulture_sam, target);
                    } else {
                        target->m_game->add_log("LOG_STOLEN_CARD", vulture_sam, target, c);
                    }
                    vulture_sam->steal_card(c);
                }
            }
        });
    }

}