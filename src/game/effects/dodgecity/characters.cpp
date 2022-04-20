#include "characters.h"

#include "../base/requests.h"
#include "../base/effects.h"

#include "../../game.h"

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
                target->m_game->pop_request_noupdate<request_draw>();
                int ncards = target->m_game->num_alive() + target->m_num_cards_to_draw - 1;
                for (int i=0; i<ncards; ++i) {
                    target->m_game->draw_phase_one_card_to(pocket_type::selection, target);
                }
                target->m_game->queue_request<request_claus_the_saint>(target_card, target);
            }
        });
    }

    player *request_claus_the_saint::get_next_target() const {
        int index = target->m_game->num_alive() - target->m_game->m_selection.size();
        auto p = target;
        for (int i=0; i<index; ++i) {
            p = target->m_game->get_next_player(p);
        }
        return p;
    }

    void request_claus_the_saint::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        if (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
            ++target->m_num_drawn_cards;
            target->add_to_hand(target_card);
            target->m_game->call_event<event_type::on_card_drawn>(target, target_card);
        } else {
            get_next_target()->add_to_hand(target_card);
        }
        if (target->m_game->m_selection.size() == 1) {
            get_next_target()->add_to_hand(target->m_game->m_selection.front());
            target->m_game->pop_request<request_claus_the_saint>();
        } else {
            target->m_game->update_request();
        }
    }

    game_formatted_string request_claus_the_saint::status_text(player *owner) const {
        if (owner == target) {
            if (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
                return {"STATUS_CLAUS_THE_SAINT_DRAW", origin_card};
            } else {
                return {"STATUS_CLAUS_THE_SAINT_GIVE", origin_card, get_next_target()};
            }
        } else if (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
            return {"STATUS_CLAUS_THE_SAINT_DRAW_OTHER", target, origin_card};
        } else if (auto p = get_next_target(); p != owner) {
            return {"STATUS_CLAUS_THE_SAINT_GIVE_OTHER", target, origin_card, p};
        } else {
            return {"STATUS_CLAUS_THE_SAINT_GIVE_YOU", target, origin_card};
        }
    }
    
    void effect_herb_hunter::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                p->m_game->draw_card_to(pocket_type::player_hand, p);
                p->m_game->draw_card_to(pocket_type::player_hand, p);
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
                p->m_game->queue_action([p]{
                    if (p->alive()) {
                        p->m_game->draw_card_to(pocket_type::player_hand, p);
                    }
                });
            }
        });
    }

    void effect_bellestar::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *target) {
            if (p == target) {
                p->m_game->add_disabler(target_card, [=](card *c) {
                    return c->pocket == pocket_type::player_table && c->owner != target;
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

    static void vera_custer_copy_character(player *target, card *c) {
        if (c != target->m_characters.back()) {
            auto copy = *c;
            copy.id = target->m_game->m_cards.first_available_id();
            copy.usages = 0;
            copy.pocket = pocket_type::player_character;
            copy.owner = target;

            if (target->m_characters.size() == 2) {
                target->m_game->add_public_update<game_update_type::remove_cards>(make_id_vector(target->m_characters | std::views::drop(1)));
                target->unequip_if_enabled(target->m_characters.back());
                target->m_game->m_cards.erase(target->m_characters.back()->id);
                target->m_characters.pop_back();
            }
            card *target_card = &target->m_game->m_cards.emplace(std::move(copy));
            target->m_characters.emplace_back(target_card);
            target->equip_if_enabled(target_card);

            target->m_game->add_public_update<game_update_type::add_cards>(
                make_id_vector(std::views::single(target_card)), pocket_type::player_character, target->id);
            target->m_game->send_card_update(target_card, target, show_card_flags::instant | show_card_flags::shown);
        }
    }

    void effect_vera_custer::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_turn_start>({target_card, 1}, [=, &usages = target_card->usages](player *target) {
            if (p == target) {
                ++usages;
                if (p->m_game->num_alive() == 2 && p->m_game->get_next_player(p)->m_characters.size() == 1) {
                    vera_custer_copy_character(p, p->m_game->get_next_player(p)->m_characters.front());
                } else if (p->m_game->num_alive() > 2) {
                    p->m_game->queue_request<request_vera_custer>(target_card, target);
                }
            }
        });
        p->m_game->add_event<event_type::on_turn_end>(target_card, [p, &usages = target_card->usages](player *target) {
            if (p == target && usages == 0) {
                if (p->m_characters.size() == 2) {
                    p->m_game->add_public_update<game_update_type::remove_cards>(make_id_vector(p->m_characters | std::views::drop(1)));
                    p->unequip_if_enabled(p->m_characters.back());
                    p->m_game->m_cards.erase(p->m_characters.back()->id);
                    p->m_characters.pop_back();
                }
            }
        });
    }

    bool request_vera_custer::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_character
            && target_player->alive() && target_player != target
            && (target_player->m_characters.size() == 1 || target_card != target_player->m_characters.front());
    }

    void request_vera_custer::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->pop_request<request_vera_custer>();
        vera_custer_copy_character(target, target_card);
    }

    game_formatted_string request_vera_custer::status_text(player *owner) const {
        if (owner == target) {
            return {"STATUS_VERA_CUSTER", origin_card};
        } else {
            return {"STATUS_VERA_CUSTER_OTHER", target, origin_card};
        }
    }

    void handler_doc_holyday::on_play(card *origin_card, player *origin, const target_list &targets) {
        card *card1 = std::get<target_card_t>(targets[0]).target;
        card *card2 = std::get<target_card_t>(targets[1]).target;
        player *target = std::get<target_player_t>(targets[2]).target;
        effect_destroy{}.on_play(origin_card, origin, card1);
        effect_destroy{}.on_play(origin_card, origin, card2);
        if (!target->immune_to(card1) || !target->immune_to(card2)) {
            effect_bang{}.on_play(origin_card, origin, target);
        }
    }

    void effect_greg_digger::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                p->heal(2);
            }
        });
    }
}