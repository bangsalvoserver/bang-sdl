#include "common/effects/characters_dodgecity.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_bill_noface::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_from_deck>(target_card, [target](player *origin) {
            if (target == origin) {
                target->m_num_cards_to_draw = target->m_num_cards_to_draw - 1 + target->m_max_hp - target->m_hp;
            }
        });
    }

    void effect_tequila_joe::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_beer_modifier>(target_card, [target](player *origin, int &value) {
            if (target == origin) {
                ++value;
            }
        });
    }

    void effect_claus_the_saint::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_from_deck>(target_card, [=](player *origin) {
            if (origin == target) {
                target->m_game->pop_request_noupdate(request_type::draw);
                int ncards = target->m_game->num_alive() + target->m_num_cards_to_draw - 1;
                for (int i=0; i<ncards; ++i) {
                    target->m_game->draw_phase_one_card_to(card_pile_type::selection, target);
                }
                target->m_game->queue_request<request_type::claus_the_saint>(target_card, target);
            }
        });
    }

    void request_claus_the_saint::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto get_next_target = [&]{
            int index = target->m_game->num_alive() - target->m_game->m_selection.size();
            auto p = target;
            for (int i=0; i<index; ++i) {
                p = target->m_game->get_next_player(p);
            }
            return p;
        };
        if (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
            ++target->m_num_drawn_cards;
            target->add_to_hand(target_card);
            target->m_game->instant_event<event_type::on_card_drawn>(target, target_card);
        } else {
            get_next_target()->add_to_hand(target_card);
        }
        if (target->m_game->m_selection.size() == 1) {
            get_next_target()->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request(request_type::claus_the_saint);
        }
    }

    game_formatted_string request_claus_the_saint::status_text() const {
        return {"STATUS_CLAUS_THE_SAINT", origin_card};
    }
    
    void effect_herb_hunter::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                p->m_game->draw_card_to(card_pile_type::player_hand, p);
                p->m_game->draw_card_to(card_pile_type::player_hand, p);
            }
        });
    }

    void effect_johnny_kisch::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_equip>(target_card, [=](player *origin, player *target, card *equipped_card) {
            if (p == origin) {
                for (auto &other : p->m_game->m_players) {
                    if (&other == target) continue;
                    if (card *card = other.find_equipped_card(equipped_card)) {
                        other.discard_card(card);
                    }
                }
            }
        });
    }

    void effect_molly_stark::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_play_hand_card>(target_card, [p](player *target, card *card) {
            if (p == target && p->m_game->m_playing != p) {
                p->m_game->draw_card_to(card_pile_type::player_hand, p);
            }
        });
    }

    void effect_bellestar::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *target) {
            if (p == target) {
                p->m_game->add_disabler(target_card, [=](card *c) {
                    return c->pile == card_pile_type::player_table && c->owner != target;
                });
            }
        });
        p->m_game->add_event<event_type::on_turn_end>(target_card, [=](player *target) {
            if (p == target) {
                p->m_game->remove_disablers(target_card);
            }
        });
    }

    void effect_bellestar::on_unequip(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
        target->m_game->remove_events(target_card);
    }

    static void vera_custer_copy_character(player *target, character *c) {
        if (c != target->m_characters.back()) {
            auto copy_it = target->m_game->m_characters.emplace(target->m_game->get_next_id(), *c).first;
            copy_it->second.id = copy_it->first;
            copy_it->second.usages = 0;
            copy_it->second.pile = card_pile_type::player_character;
            copy_it->second.owner = target;

            if (target->m_characters.size() == 2) {
                target->m_characters.back()->on_unequip(target);
                target->m_game->m_characters.erase(target->m_characters.back()->id);
                target->m_characters.pop_back();
            }
            target->m_game->add_public_update<game_update_type::player_clear_characters>(target->id);
            target->m_game->send_character_update(copy_it->second, target->id, 1);
            target->m_characters.emplace_back(&copy_it->second)->on_equip(target);
        }
    }

    void effect_vera_custer::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::before_turn_start>(target_card, [=](player *target) {
            if (p == target) {
                ++p->m_characters.front()->usages;
                if (p->m_game->num_alive() == 2 && p->m_game->get_next_player(p)->m_characters.size() == 1) {
                    vera_custer_copy_character(p, p->m_game->get_next_player(p)->m_characters.front());
                } else if (p->m_game->num_alive() > 2) {
                    p->m_game->queue_request<request_type::vera_custer>(target_card, target);
                }
            }
        });
        p->m_game->add_event<event_type::on_turn_end>(target_card, [p](player *target) {
            if (p == target && p->m_characters.front()->usages == 0) {
                if (p->m_characters.size() > 1) {
                    auto *c = p->m_characters.back();
                    c->on_unequip(p);
                    c->pile = card_pile_type::none;
                    c->owner = nullptr;
                    p->m_characters.pop_back();
                    p->m_game->add_public_update<game_update_type::player_clear_characters>(p->id);
                    p->m_game->m_characters.erase(c->id);
                }
            }
        });
    }

    void request_vera_custer::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_card != target->m_characters.front()) {
            target->m_game->pop_request(request_type::vera_custer);
            vera_custer_copy_character(target, static_cast<character *>(target_card));
        }
    }

    game_formatted_string request_vera_custer::status_text() const {
        return {"STATUS_VERA_CUSTER", origin_card};
    }
}