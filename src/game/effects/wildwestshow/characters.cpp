#include "characters.h"

#include "../base/requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_big_spencer::on_enable(card *target_card, player *p) {
        p->m_game->add_disabler(target_card, [=](card *c) {
            return c->pocket == pocket_type::player_hand
                && c->owner == p
                && c->responses.last_is(effect_type::missedcard);
        });
        p->m_game->add_event<event_type::apply_initial_cards_modifier>(target_card, [=](player *target, int &value) {
            if (p == target) {
                value = 5;
            }
        });
    }

    void effect_big_spencer::on_disable(card *target_card, player *p) {
        p->m_game->remove_disablers(target_card);
        p->m_game->remove_events(target_card);
    }

    void effect_gary_looter::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_discard_pass>(target_card, [p](player *origin, card *discarded_card) {
            if (p != origin) {
                for (;origin != p; origin = origin->m_game->get_next_player(origin)) {
                    if (std::ranges::any_of(origin->m_characters, [](const card *c) {
                        return c->equips.last_is(equip_type::gary_looter);
                    })) {
                        return;
                    }
                }
                p->m_game->add_log("LOG_DRAWN_CARD", p, discarded_card);
                p->m_game->move_card(discarded_card, pocket_type::player_hand, p, show_card_flags::short_pause);
            }
        });
    }

    void effect_john_pain::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_draw_check>(target_card, [player_end = p](player *player_begin, card *drawn_card) {
            if (player_end->alive() && player_end->m_hand.size() < 6) {
                for (player *it = player_begin; it != player_end; it = it->m_game->get_next_player(it)) {
                    if (std::ranges::any_of(it->m_characters, [](const card *c) {
                        return c->equips.last_is(equip_type::john_pain);
                    })) {
                        return;
                    }
                }
                player_end->m_game->add_log("LOG_DRAWN_CARD", player_end, drawn_card);
                player_end->m_game->move_card(drawn_card, pocket_type::player_hand, player_end, show_card_flags::short_pause);
            }
        });
    }

    bool effect_teren_kill::can_respond(card *origin_card, player *origin) const {
        if (auto *req = origin->m_game->top_request_if<request_death>(origin)) {
            return std::ranges::find(req->draw_attempts, origin_card) == req->draw_attempts.end();
        }
        return false;
    }

    void effect_teren_kill::on_play(card *origin_card, player *origin) {
        origin->m_game->top_request().get<request_death>().draw_attempts.push_back(origin_card);
        origin->m_game->draw_check_then(origin, origin_card, [origin, origin_card](card *drawn_card) {
            if (origin->get_card_sign(drawn_card).suit != card_suit::spades) {
                origin->m_game->pop_request<request_death>();
                origin->m_hp = 1;
                origin->m_game->add_update<game_update_type::player_hp>(origin->id, 1);
                origin->draw_card(1, origin_card);
            }
        });
    }

    void effect_youl_grinner::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *origin) {
            if (target == origin) {
                for (auto &p : target->m_game->m_players) {
                    if (p.alive() && p.id != target->id && p.m_hand.size() > target->m_hand.size()) {
                        target->m_game->queue_request<request_youl_grinner>(target_card, target, &p);
                    }
                }
            }
        });
    }

    bool request_youl_grinner::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_hand && target_player == target;
    }

    void request_youl_grinner::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->pop_request<request_youl_grinner>();
        target->m_game->add_log(update_target::includes(origin, target), "LOG_GIFTED_CARD", target, origin, target_card);
        target->m_game->add_log(update_target::excludes(origin, target), "LOG_GIFTED_A_CARD", target, origin);
        origin->steal_card(target_card);
        target->m_game->call_event<event_type::on_effect_end>(origin, origin_card);
    }

    game_formatted_string request_youl_grinner::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_YOUL_GRINNER", origin_card};
        } else {
            return {"STATUS_YOUL_GRINNER_OTHER", target, origin_card};
        }
    }

    void handler_flint_westwood::on_play(card *origin_card, player *origin, const target_list &targets) {
        auto chosen_card = std::get<target_card_t>(targets[0]).target;
        auto target = std::get<target_card_t>(targets[1]).target->owner;

        for (int i=2; i && !target->m_hand.empty(); --i) {
            card *stolen_card = target->random_hand_card();
            target->m_game->add_log(update_target::includes(origin, target), "LOG_STOLEN_CARD", origin, target, stolen_card);
            target->m_game->add_log(update_target::excludes(origin, target), "LOG_STOLEN_CARD_FROM_HAND", origin, target);
            origin->steal_card(stolen_card);
        }
        target->m_game->add_log(update_target::includes(origin, target), "LOG_GIFTED_CARD", origin, target, chosen_card);
        target->m_game->add_log(update_target::excludes(origin, target), "LOG_GIFTED_A_CARD", origin, target);
        target->steal_card(chosen_card);
    }

    void effect_greygory_deck::on_equip(card *target_card, player *target) {
        std::vector<card *> base_characters;
        for (card &c : target->m_game->m_cards) {
            if (c.expansion == card_expansion_type{}
                && (c.pocket == pocket_type::none
                || (c.pocket == pocket_type::player_character && c.owner == target)))
                base_characters.push_back(&c);
        }
        std::ranges::shuffle(base_characters, target->m_game->rng);

        target->m_game->add_update<game_update_type::add_cards>(
            make_id_vector(base_characters | std::views::take(2)),
            pocket_type::player_character, target->id);
        for (int i=0; i<2; ++i) {
            auto *c = target->m_characters.emplace_back(base_characters[i]);
            target->m_game->add_log("LOG_CHARACTER_CHOICE", target, c);
            c->pocket = pocket_type::player_character;
            c->owner = target;
            c->on_enable(target);
            target->m_game->send_card_update(c, target, show_card_flags::instant | show_card_flags::shown);
        }
    }
    
    void effect_greygory_deck::on_play(card *target_card, player *target) {
        for (int i=1; i<target->m_characters.size(); ++i) {
            auto *c = target->m_characters[i];
            target->disable_equip(c);
            c->pocket = pocket_type::none;
            c->owner = nullptr;
        }
        if (target->m_characters.size() > 1) {
            target->m_game->add_update<game_update_type::remove_cards>(make_id_vector(target->m_characters | std::views::drop(1)));
            target->m_characters.resize(1);
        }
        on_equip(target_card, target);
        target->m_game->update_request();
    }

}