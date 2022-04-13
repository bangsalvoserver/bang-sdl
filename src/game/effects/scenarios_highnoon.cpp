#include "scenarios_highnoon.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_blessing::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_sign_modifier>(target_card, [](player *, card_sign &value) {
            value.suit = card_suit::hearts;
        });
    }

    void effect_curse::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_sign_modifier>(target_card, [](player *, card_sign &value) {
            value.suit = card_suit::spades;
        });
    }

    void effect_thedaltons::on_equip(card *target_card, player *target) {
        player *p = target;
        while(true) {
            if (std::ranges::find(p->m_table, card_color_type::blue, &card::color) != p->m_table.end()) {
                p->m_game->queue_request<request_thedaltons>(target_card, p);
            }

            p = target->m_game->get_next_player(p);
            if (p == target) break;
        }
    }

    bool request_thedaltons::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_table
            && target == target_player
            && target_card->color == card_color_type::blue;
    }

    void request_thedaltons::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->discard_card(target_card);
        target->m_game->pop_request<request_thedaltons>();
    }

    game_formatted_string request_thedaltons::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_THEDALTONS", origin_card};
        } else {
            return {"STATUS_THEDALTONS_OTHER", target, origin_card};
        }
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
            return c->pocket == pocket_type::player_hand
                && !c->effects.empty()
                && c->effects.front().is(effect_type::beer);
        });
    }

    void effect_reverend::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
    }

    void effect_hangover::on_equip(card *target_card, player *target) {
        target->m_game->add_disabler(target_card, [](card *c) {
            return c->pocket == pocket_type::player_character;
        });
    }

    void effect_hangover::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
        target->m_game->call_event<event_type::on_effect_end>(target, target_card);
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
                    it = origin->m_game->move_to(card, pocket_type::selection, true, nullptr, show_card_flags::no_animation);
                } else {
                    ++it;
                }
            }
            origin->m_game->queue_request<request_handcuffs>(target_card, origin);
        });
        target->m_game->add_event<event_type::on_turn_end>(target_card, [target_card](player *p) {
            p->m_game->remove_disablers(target_card);
        });
    }

    void request_handcuffs::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->add_disabler(origin_card, 
            [target=target, declared_suit = static_cast<card_suit>(target_card->responses.front().effect_value)]
            (card *c) {
                return c->owner == target && c->sign && c->sign.suit != declared_suit;
            });

        while (!target->m_game->m_selection.empty()) {
            target->m_game->move_to(target->m_game->m_selection.front(), pocket_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }
        target->m_game->pop_request<request_handcuffs>();
    }

    game_formatted_string request_handcuffs::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_HANDCUFFS", origin_card};
        } else {
            return {"STATUS_HANDCUFFS_OTHER", target, origin_card};
        }
    }

    void effect_newidentity::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            target->m_game->move_to(p->m_backup_character.front(), pocket_type::selection, true, nullptr);
            target->m_game->queue_request<request_newidentity>(target_card, p);
        });
    }

    bool request_newidentity::can_pick(pocket_type pocket, player *, card *target_card) const {
        return pocket == pocket_type::selection
            || (pocket == pocket_type::player_character && target_card == target->m_characters.front());
    }

    void request_newidentity::on_pick(pocket_type pocket, player *, card *target_card) {
        if (pocket == pocket_type::selection) {
            for (card *c : target->m_characters) {
                target->unequip_if_enabled(c);
            }
            if (target->m_characters.size() > 1) {
                target->m_game->add_public_update<game_update_type::remove_cards>(make_id_vector(target->m_characters | std::views::drop(1)));
                target->m_characters.resize(1);
            }

            card *old_character = target->m_characters.front();
            target->m_game->move_to(old_character, pocket_type::player_backup, false, target);
            target->m_game->move_to(target_card, pocket_type::player_character, true, target, show_card_flags::show_everyone);

            target->equip_if_enabled(target_card);
            target->move_cubes(old_character, target_card, old_character->cubes.size());
            for (auto &e : target_card->equips) {
                e.on_pre_equip(target_card, target);
            }
            
            target->m_hp = 2;
            target->m_game->add_public_update<game_update_type::player_hp>(target->id, target->m_hp, false, false);
        } else {
            target->m_game->move_to(target->m_game->m_selection.front(), pocket_type::player_backup, false, target);
        }
        target->m_game->pop_request<request_newidentity>();
    }

    game_formatted_string request_newidentity::status_text(player *owner) const {
        if (target == owner) {
            return {"STATUS_NEWIDENTITY", origin_card};
        } else {
            return {"STATUS_NEWIDENTITY_OTHER", target, origin_card};
        }
    }
}