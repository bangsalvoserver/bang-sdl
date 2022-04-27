#include "characters.h"

#include "../base/requests.h"
#include "../base/effects.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_bill_noface::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_from_deck>(target_card, [target](player *origin) {
            if (target == origin) {
                target->m_num_cards_to_draw = target->m_num_cards_to_draw - 1 + target->m_max_hp - target->m_hp;
            }
        });
    }

    void effect_tequila_joe::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_beer_modifier>(target_card, [target](player *origin, int &value) {
            if (target == origin) {
                ++value;
            }
        });
    }

    void effect_claus_the_saint::on_enable(card *target_card, player *target) {
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
            target->m_game->add_log(update_target::includes(target), "LOG_DRAWN_CARD", target, target_card);
            target->m_game->add_log(update_target::excludes(target), "LOG_DRAWN_A_CARD", target);
            target->add_to_hand(target_card);
            target->m_game->call_event<event_type::on_card_drawn>(target, target_card);
        } else {
            player *next_target = get_next_target();
            target->m_game->add_log(update_target::includes(target, next_target), "LOG_GIFTED_CARD", target, next_target, target_card);
            target->m_game->add_log(update_target::excludes(target, next_target), "LOG_GIFTED_A_CARD", target, next_target);
            next_target->add_to_hand(target_card);
        }
        if (target->m_game->m_selection.size() == 1) {
            player *next_target = get_next_target();
            card *last_card = target->m_game->m_selection.front();
            target->m_game->add_log(update_target::includes(target, next_target), "LOG_GIFTED_CARD", target, next_target, last_card);
            target->m_game->add_log(update_target::excludes(target, next_target), "LOG_GIFTED_A_CARD", target, last_card);
            next_target->add_to_hand(last_card);
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
    
    void effect_herb_hunter::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p, target_card](player *origin, player *target) {
            if (p != target) {
                p->draw_card(2, target_card);
            }
        });
    }

    void effect_johnny_kisch::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_equip_card>(target_card, [=](player *origin, player *target, card *equipped_card) {
            if (p == origin) {
                for (auto &other : p->m_game->m_players) {
                    if (&other == target) continue;
                    if (card *card = other.find_equipped_card(equipped_card)) {
                        target->m_game->add_log("LOG_DISCARDED_CARD_FOR", target_card, &other, card);
                        other.discard_card(card);
                    }
                }
            }
        });
    }

    void effect_molly_stark::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_play_hand_card>(target_card, [p, target_card](player *target, card *card) {
            if (p == target && p->m_game->m_playing != p) {
                p->m_game->queue_action([p, target_card]{
                    if (p->alive()) {
                        p->draw_card(1, target_card);
                    }
                });
            }
        });
    }

    void effect_bellestar::on_enable(card *target_card, player *p) {
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

    void effect_bellestar::on_disable(card *target_card, player *target) {
        target->m_game->remove_disablers(target_card);
        target->m_game->remove_events(target_card);
    }

    void effect_vera_custer::copy_characters(player *origin, player *target) {
        remove_characters(origin);

        std::ranges::for_each(
            target->m_characters
                | std::views::reverse
                | std::views::take(2)
                | std::views::reverse,
            [origin](card *target_card) {
                origin->m_game->add_log("LOG_COPY_CHARACTER", origin, target_card);
                
                auto card_copy = std::make_unique<card>(static_cast<const card_data &>(*target_card));
                card_copy->id = origin->m_game->m_cards.first_available_id();
                card_copy->pocket = pocket_type::player_character;
                card_copy->owner = origin;

                card *card_ptr = origin->m_game->m_cards.insert(std::move(card_copy)).get();
                
                origin->m_characters.emplace_back(card_ptr);
                card_ptr->on_enable(origin);

                origin->m_game->add_update<game_update_type::add_cards>(
                    make_id_vector(std::views::single(card_ptr)), pocket_type::player_character, origin->id);
                origin->m_game->send_card_update(card_ptr, origin, show_card_flags::instant | show_card_flags::shown);
            });
    }

    void effect_vera_custer::remove_characters(player *origin) {
        if (origin->m_characters.size() > 1) {
            origin->m_game->add_update<game_update_type::remove_cards>(make_id_vector(origin->m_characters | std::views::drop(1)));
            while (origin->m_characters.size() > 1) {
                origin->disable_equip(origin->m_characters.back());
                origin->m_game->m_cards.erase(origin->m_characters.back()->id);
                origin->m_characters.pop_back();
            }
        }
    }

    void effect_vera_custer::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::on_turn_start>({origin_card, 1}, [=, &usages = origin_card->usages](player *target) {
            if (origin == target) {
                ++usages;
                if (origin->m_game->num_alive() == 2) {
                    copy_characters(origin, origin->m_game->get_next_player(origin));
                } else {
                    origin->m_game->queue_request<request_vera_custer>(origin_card, target);
                }
            }
        });
        origin->m_game->add_event<event_type::on_turn_end>(origin_card, [origin, &usages = origin_card->usages](player *target) {
            if (origin == target && usages == 0) {
                remove_characters(origin);
            }
        });
    }

    bool request_vera_custer::can_pick(pocket_type pocket, player *target_player, card *target_card) const {
        return pocket == pocket_type::player_character
            && target_player->alive() && target_player != target;
    }

    void request_vera_custer::on_pick(pocket_type pocket, player *target_player, card *target_card) {
        target->m_game->pop_request<request_vera_custer>();
        effect_vera_custer::copy_characters(target, target_player);
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

    void effect_greg_digger::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (p != target) {
                p->heal(2);
            }
        });
    }
}