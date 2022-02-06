#include "common/effects/characters_wildwestshow.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_big_spencer::on_equip(card *target_card, player *p) {
        p->m_game->add_disabler(target_card, [=](card *c) {
            return c->pile == card_pile_type::player_hand
                && c->owner == p
                && !c->responses.empty()
                && c->responses.front().is(effect_type::missedcard);
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
                    if (std::ranges::any_of(origin->m_characters, [](const character *c) {
                        return !c->equips.empty() && c->equips.front().is(equip_type::gary_looter);
                    })) {
                        return;
                    }
                }
                p->add_to_hand(discarded_card);
            }
        });
    }

    void effect_john_pain::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_draw_check>(target_card, [p](player *origin, card *drawn_card) {
            if (p->m_hand.size() < 6) {
                for (;origin != p; origin = origin->m_game->get_next_player(origin)) {
                    if (std::ranges::any_of(origin->m_characters, [](const character *c) {
                        return !c->equips.empty() && c->equips.front().is(equip_type::john_pain);
                    })) {
                        return;
                    }
                }
                p->add_to_hand(drawn_card);
            }
        });
    }

    bool effect_teren_kill::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::death, origin)) {
            const auto &vec = origin->m_game->top_request().get<request_type::death>().draw_attempts;
            return std::ranges::find(vec, origin_card) == vec.end();
        }
        return false;
    }

    void effect_teren_kill::on_play(card *origin_card, player *origin) {
        origin->m_game->top_request().get<request_type::death>().draw_attempts.push_back(origin_card);
        origin->m_game->draw_check_then(origin, origin_card, [origin](card *drawn_card) {
            if (origin->get_card_suit(drawn_card) != card_suit_type::spades) {
                origin->m_game->pop_request(request_type::death);
                origin->m_hp = 1;
                origin->m_game->add_public_update<game_update_type::player_hp>(origin->id, 1);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
            }
        });
    }

    void effect_youl_grinner::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *origin) {
            if (target == origin) {
                for (auto &p : target->m_game->m_players) {
                    if (p.alive() && p.id != target->id && p.m_hand.size() > target->m_hand.size()) {
                        target->m_game->queue_request<request_type::youl_grinner>(target_card, target, &p);
                    }
                }
            }
        });
    }

    bool request_youl_grinner::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_hand && target_player == target;
    }

    void request_youl_grinner::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_game->pop_request(request_type::youl_grinner);
        origin->steal_card(target, target_card);
        target->m_game->queue_event<event_type::on_effect_end>(origin, origin_card);
    }

    game_formatted_string request_youl_grinner::status_text() const {
        return {"STATUS_YOUL_GRINNER", origin_card};
    }

    struct flint_westwood_handler {
        card *chosen_card;

        void operator()(player *, card *) {}
    };
    
    void effect_flint_westwood_choose::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card, flint_westwood_handler{target_card});
    }

    void effect_flint_westwood::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        int num_cards = 2;
        for (int i=0; !target->m_hand.empty() && i<2; ++i) {
            origin->steal_card(target, target->random_hand_card());
        }
        target->steal_card(origin, origin->m_game->m_event_handlers.find(origin_card)->
            second.get<event_type::on_play_card_end>().target<flint_westwood_handler>()->chosen_card);
        origin->m_game->remove_events(origin_card);
    }

    static void greygory_deck_set_characters(player *target) {
        std::ranges::shuffle(target->m_game->m_base_characters, target->m_game->rng);
        for (int i=0; i<2; ++i) {
            auto *c = target->m_characters.emplace_back(target->m_game->m_base_characters[i]);
            c->on_equip(target);
            c->pile = card_pile_type::player_character;
            c->owner = target;
            target->m_game->send_character_update(*c, target->id, i+1);
        }
    }
    
    void effect_greygory_deck::on_play(card *target_card, player *target) {
        for (int i=1; i<target->m_characters.size(); ++i) {
            auto *c = target->m_characters[i];
            c->on_unequip(target);
            c->pile = card_pile_type::none;
            c->owner = nullptr;
        }
        target->m_characters.resize(1);
        target->m_game->add_public_update<game_update_type::player_clear_characters>(target->id);
        greygory_deck_set_characters(target);
    }

    void effect_greygory_deck::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_game_start>(target_card, [p] {
            greygory_deck_set_characters(p);
        });
    }

}