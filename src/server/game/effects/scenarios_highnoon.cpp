#include "common/effects/scenarios_highnoon.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_blessing::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_suit_modifier>(target_card, [](card_suit_type &suit) {
            suit = card_suit_type::hearts;
        });
    }

    void effect_curse::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_suit_modifier>(target_card, [](card_suit_type &suit) {
            suit = card_suit_type::spades;
        });
    }

    void effect_thedaltons::on_equip(card *target_card, player *target) {
        player *p = target;
        while(true) {
            if (std::ranges::find(p->m_table, card_color_type::blue, &card::color) != p->m_table.end()) {
                p->m_game->queue_request<request_type::thedaltons>(target_card, p);
            }

            p = target->m_game->get_next_player(p);
            if (p == target) break;
        }
    }

    bool request_thedaltons::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return pile == card_pile_type::player_table
            && target == target_player
            && target_card->color == card_color_type::blue;
    }

    void request_thedaltons::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->discard_card(target_card);
        target->m_game->pop_request(request_type::thedaltons);
    }

    game_formatted_string request_thedaltons::status_text() const {
        return {"STATUS_THEDALTONS", origin_card};
    }

    void effect_thedoctor::on_equip(card *target_card, player *target) {
        int min_hp = std::ranges::min(target->m_game->m_players
            | std::views::filter(&player::alive)
            | std::views::transform(&player::m_hp));
        
        for (auto &p : target->m_game->m_players) {
            if (p.m_hp == min_hp) {
                p.heal(1);
            }
        }
    }

    void effect_trainarrival::on_equip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_trainarrival::on_unequip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_equip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_unequip(card *target_card, player *target) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_highnoon::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            p->damage(target_card, nullptr, 1);
        });
    }

    void effect_shootout::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [](player *p) {
            ++p->m_bangs_per_turn;
        });
    }

    void effect_invert_rotation::on_equip(card *target_card, player *target) {
        target->m_game->m_scenario_flags |= scenario_flags::invert_rotation;
    }

    void effect_reverend::on_equip(card *target_card, player *target) {
        target->m_game->add_disabler(target_card, [](card *c) {
            return c->pile == card_pile_type::player_hand
                && !c->effects.empty()
                && c->effects.front().is(effect_type::beer);
        });
    }

    void effect_reverend::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
    }

    void effect_hangover::on_equip(card *target_card, player *target) {
        target->m_game->add_disabler(target_card, [](card *c) {
            return c->pile == card_pile_type::player_character;
        });
    }

    void effect_hangover::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
        target->m_game->queue_event<event_type::on_effect_end>(target, target_card);
    }

    void effect_sermon::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            target->m_game->add_disabler(target_card, [=](card *c) {
                return c->owner == p
                    && std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::type) != c->effects.end();
            });
        });
        target->m_game->add_event<event_type::on_turn_end>(target_card, [=](player *p) {
            target->m_game->remove_disablers(target_card);
        });
    }

    void effect_sermon::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
        target->m_game->remove_events(target_card);
    }

    void effect_ghosttown::on_equip(card *target_card, player *target) {
        target->m_game->m_scenario_flags |= scenario_flags::ghosttown;
    }

    void effect_handcuffs::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::post_draw_cards>(target_card, [=](player *origin) {
            auto &vec = origin->m_game->m_hidden_deck;
            for (auto it = vec.begin(); it != vec.end(); ) {
                auto *card = *it;
                if (!card->responses.empty() && card->responses.front().is(effect_type::handcuffschoice)) {
                    it = origin->m_game->move_to(card, card_pile_type::selection, true, nullptr, show_card_flags::no_animation);
                } else {
                    ++it;
                }
            }
            origin->m_game->queue_request<request_type::handcuffs>(target_card, origin);
        });
    }

    void request_handcuffs::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_declared_suit = static_cast<card_suit_type>(target_card->responses.front().args);

        while (!target->m_game->m_selection.empty()) {
            target->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }
        target->m_game->pop_request(request_type::handcuffs);
    }

    game_formatted_string request_handcuffs::status_text() const {
        return {"STATUS_HANDCUFFS", origin_card};
    }
}