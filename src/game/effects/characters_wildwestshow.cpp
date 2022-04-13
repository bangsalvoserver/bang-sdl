#include "characters_wildwestshow.h"
#include "requests_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_big_spencer::on_equip(card *target_card, player *p) {
        p->m_game->add_disabler(target_card, [=](card *c) {
            return c->pocket == pocket_type::player_hand
                && c->owner == p
                && !c->responses.empty()
                && c->responses.back().is(effect_type::missedcard);
        });
        p->m_game->add_event<event_type::apply_initial_cards_modifier>(target_card, [=](player *target, int &value) {
            if (p == target) {
                value = 5;
            }
        });
    }

    void effect_big_spencer::on_unequip(card *target_card, player *p) {
        p->m_game->remove_disablers(target_card);
        p->m_game->remove_events(target_card);
    }

    void effect_gary_looter::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_discard_pass>(target_card, [p](player *origin, card *discarded_card) {
            if (p != origin) {
                for (;origin != p; origin = origin->m_game->get_next_player(origin)) {
                    if (std::ranges::any_of(origin->m_characters, [](const card *c) {
                        return !c->equips.empty() && c->equips.front().is(equip_type::gary_looter);
                    })) {
                        return;
                    }
                }
                p->m_game->move_to(discarded_card, pocket_type::player_hand, true, p, show_card_flags::short_pause);
            }
        });
    }

    void effect_john_pain::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_draw_check>(target_card, [p](player *origin, card *drawn_card) {
            p->m_game->queue_action([player_begin = origin, player_end = p, drawn_card] {
                if (player_end->alive() && player_end->m_hand.size() < 6) {
                    for (player *it = player_begin; it != player_end; it = it->m_game->get_next_player(it)) {
                        if (std::ranges::any_of(it->m_characters, [](const card *c) {
                            return !c->equips.empty() && c->equips.front().is(equip_type::john_pain);
                        })) {
                            return;
                        }
                    }
                    player_end->m_game->move_to(drawn_card, pocket_type::player_hand, true, player_end, show_card_flags::short_pause);
                }
            });
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
        origin->m_game->draw_check_then(origin, origin_card, [origin](card *drawn_card) {
            if (origin->get_card_sign(drawn_card).suit != card_suit::spades) {
                origin->m_game->pop_request<request_death>();
                origin->m_hp = 1;
                origin->m_game->add_public_update<game_update_type::player_hp>(origin->id, 1);
                origin->m_game->draw_card_to(pocket_type::player_hand, origin);
            }
        });
    }

    void effect_youl_grinner::on_equip(card *target_card, player *target) {
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
        origin->steal_card(target, target_card);
        target->m_game->call_event<event_type::on_effect_end>(origin, origin_card);
    }

    game_formatted_string request_youl_grinner::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_YOUL_GRINNER", origin_card};
        } else {
            return {"STATUS_YOUL_GRINNER_OTHER", target, origin_card};
        }
    }

    void handler_flint_westwood::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        auto chosen_card = std::get<card *>(targets[0]);
        auto target = std::get<player *>(targets[1]);

        for (int i=2; i && !target->m_hand.empty(); --i) {
            origin->steal_card(target, target->random_hand_card());
        }
        target->steal_card(origin, chosen_card);
    }

    void effect_greygory_deck::on_pre_equip(card *target_card, player *target) {
        auto view = target->m_game->m_cards
            | std::views::filter([&](const card &c) {
                return c.expansion == card_expansion_type::base
                    && (c.pocket == pocket_type::none
                    || (c.pocket == pocket_type::player_character && c.owner == target));
            })
            | std::views::transform([](card &c) {
                return &c;
            });
        std::vector<card *> base_characters(view.begin(), view.end());
        std::ranges::shuffle(base_characters, target->m_game->rng);

        target->m_game->add_public_update<game_update_type::add_cards>(
            make_id_vector(base_characters | std::views::take(2)),
            pocket_type::player_character, target->id);
        for (int i=0; i<2; ++i) {
            auto *c = target->m_characters.emplace_back(base_characters[i]);
            target->equip_if_enabled(c);
            c->pocket = pocket_type::player_character;
            c->owner = target;
            target->m_game->send_card_update(*c, target, show_card_flags::no_animation | show_card_flags::show_everyone);
        }
    }
    
    void effect_greygory_deck::on_play(card *target_card, player *target) {
        for (int i=1; i<target->m_characters.size(); ++i) {
            auto *c = target->m_characters[i];
            target->unequip_if_enabled(c);
            c->pocket = pocket_type::none;
            c->owner = nullptr;
        }
        if (target->m_characters.size() > 1) {
            target->m_game->add_public_update<game_update_type::remove_cards>(make_id_vector(target->m_characters | std::views::drop(1)));
            target->m_characters.resize(1);
        }
        on_pre_equip(target_card, target);
        target->m_game->update_request();
    }

}