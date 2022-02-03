#include "game.h"

#include "common/requests.h"
#include "common/net_enums.h"

#include <array>

namespace banggame {

    void game::send_request_respond() {
        const auto &req = top_request();
        for (auto &p : m_players) {
            add_private_update<game_update_type::request_respond>(&p, [&]{
                std::vector<int> ids;
                if (!req.target() || req.target() == &p) {
                    auto add_ids_for = [&](auto &&cards) {
                        for (card *c : cards) {
                            if (std::ranges::any_of(c->responses, [&](const effect_holder &e) {
                                return e.can_respond(c, &p);
                            })) ids.push_back(c->id);
                        }
                    };
                    add_ids_for(p.m_hand | std::views::filter([](card *c) { return c->color == card_color_type::brown; }));
                    if (!table_cards_disabled(&p)) add_ids_for(p.m_table | std::views::filter(std::not_fn(&card::inactive)));
                    if (!characters_disabled(&p)) add_ids_for(p.m_characters);
                    add_ids_for(m_specials);
                }
                return ids;
            }());
        }
    }

    void game::send_request_update() {
        const auto &req = top_request();
        add_public_update<game_update_type::request_status>(
            req.enum_index(),
            req.origin() ? req.origin()->id : 0,
            req.target() ? req.target()->id : 0,
            req.flags(),
            req.status_text()
        );
        send_request_respond();
    }

    std::vector<card *> &game::get_pile(card_pile_type pile, player *owner) {
        switch (pile) {
        case card_pile_type::player_hand:       return owner->m_hand;
        case card_pile_type::player_table:      return owner->m_table;
        case card_pile_type::main_deck:         return m_deck;
        case card_pile_type::discard_pile:      return m_discards;
        case card_pile_type::selection:         return m_selection;
        case card_pile_type::shop_deck:         return m_shop_deck;
        case card_pile_type::shop_selection:    return m_shop_selection;
        case card_pile_type::shop_discard:      return m_shop_discards;
        case card_pile_type::hidden_deck:       return m_hidden_deck;
        case card_pile_type::scenario_deck:     return m_scenario_deck;
        case card_pile_type::scenario_card:     return m_scenario_cards;
        case card_pile_type::specials:          return m_specials;
        default: throw std::runtime_error("Invalid Pile");
        }
    }

    std::vector<card *>::iterator game::move_to(card *c, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (known) {
            send_card_update(*c, owner, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c->id, flags);
        }
        add_public_update<game_update_type::move_card>(c->id, owner ? owner->id : 0, pile, flags);
        auto &prev_pile = get_pile(c->pile, c->owner);
        get_pile(pile, owner).emplace_back(c);
        c->pile = pile;
        c->owner = owner;
        return prev_pile.erase(std::ranges::find(prev_pile, c));
    }

    card *game::draw_card_to(card_pile_type pile, player *owner, show_card_flags flags) {
        card *drawn_card = m_deck.back();
        move_to(drawn_card, pile, true, owner, flags);
        if (m_deck.empty()) {
            card *top_discards = m_discards.back();
            m_discards.resize(m_discards.size()-1);
            m_deck = std::move(m_discards);
            for (card *c : m_deck) {
                c->pile = card_pile_type::main_deck;
                c->owner = nullptr;
            }
            m_discards.clear();
            m_discards.emplace_back(top_discards);
            shuffle_cards_and_ids(m_deck);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::main_deck);
            add_log("LOG_DECK_RESHUFFLED");
        }
        return drawn_card;
    }

    card *game::draw_phase_one_card_to(card_pile_type pile, player *owner, show_card_flags flags) {
        if (!has_scenario(scenario_flags::abandonedmine) || m_discards.empty()) {
            return draw_card_to(pile, owner, flags);
        } else {
            card *drawn_card = m_discards.back();
            move_to(drawn_card, pile, true, owner, flags);
            return drawn_card;
        }
    }

    card *game::draw_shop_card() {
        card *drawn_card = m_shop_deck.back();
        move_to(drawn_card, card_pile_type::shop_selection);
        if (m_shop_deck.empty()) {
            m_shop_deck = std::move(m_shop_discards);
            for (card *c : m_shop_deck) {
                c->pile = card_pile_type::shop_deck;
                c->owner = nullptr;
            }
            m_shop_discards.clear();
            shuffle_cards_and_ids(m_shop_deck);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::shop_deck);
        }
        return drawn_card;
    }

    void game::draw_scenario_card() {
        if (m_scenario_deck.empty()) return;

        if (m_scenario_deck.size() > 1) {
            send_card_update(**(m_scenario_deck.rbegin() + 1), nullptr, show_card_flags::no_animation);
        }
        if (!m_scenario_cards.empty()) {
            m_scenario_cards.back()->on_unequip(m_first_player);
            m_scenario_flags = enums::flags_none<scenario_flags>;
        }
        move_to(m_scenario_deck.back(), card_pile_type::scenario_card);
        m_scenario_cards.back()->on_equip(m_first_player);
    }
    
    void game::draw_check_then(player *origin, card *origin_card, draw_check_function fun) {
        m_current_check.emplace(std::move(fun), origin, origin_card);
        do_draw_check();
    }

    void game::do_draw_check() {
        if (m_current_check->origin->m_num_checks == 1) {
            auto *c = draw_card_to(card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(m_current_check->origin, c);
            add_log("LOG_CHECK_DREW_CARD", m_current_check->origin_card, m_current_check->origin, c);
            instant_event<event_type::trigger_tumbleweed>(m_current_check->origin_card, c);
            if (!m_current_check->no_auto_resolve) {
                m_current_check->function(c);
                m_current_check.reset();
            }
        } else {
            for (int i=0; i<m_current_check->origin->m_num_checks; ++i) {
                draw_card_to(card_pile_type::selection);
            }
            add_request<request_type::check>(m_current_check->origin_card, m_current_check->origin);
        }
    }

    void game::pop_request_noupdate(request_type type) {
        if (type != request_type::none && !top_request_is(type)) return;
        enums::visit([](auto &value) {
            if constexpr (requires { value.cleanup(); }) {
                value.cleanup();
            }
        }, m_requests.front());
        m_requests.pop_front();
    }

    player *game::get_next_player(player *p) {
        auto it = m_players.begin() + (p - m_players.data());
        do {
            if (++it == m_players.end()) it = m_players.begin();
        } while(!it->alive());
        return &*it;
    }

    player *game::get_next_in_turn(player *p) {
        auto it = m_players.begin() + (p - m_players.data());
        do {
            if (has_scenario(scenario_flags::invert_rotation)) {
                if (it == m_players.begin()) it = m_players.end();
                --it;
            } else {
                ++it;
                if (it == m_players.end()) it = m_players.begin();
            }
        } while(!it->alive() && !has_scenario(scenario_flags::ghosttown)
            && !(has_scenario(scenario_flags::deadman) && &*it == m_first_dead));
        return &*it;
    }

    int game::calc_distance(player *from, player *to) {
        if (from == to) return 0;
        if (from->check_player_flags(player_flags::see_everyone_range_1)) return 1;
        int d1=0, d2=0;
        for (player *counter = from; counter != to; counter = get_next_player(counter), ++d1);
        for (player *counter = to; counter != from; counter = get_next_player(counter), ++d2);
        return std::min(d1, d2) + to->m_distance_mod;
    }

    void game::check_game_over(player *killer, player *target) {
        if (killer != m_playing) killer = nullptr;
        
        auto winner_role = [&]{
            auto alive_players_view = m_players | std::views::filter(&player::alive);
            int num_alive = std::ranges::distance(alive_players_view);
            if (std::ranges::distance(alive_players_view) == 1 || std::ranges::all_of(alive_players_view, [](player_role role) {
                return role == player_role::sheriff || role == player_role::deputy;
            }, &player::m_role)) {
                return alive_players_view.front().m_role;
            } else if (m_players.size() > 3) {
                if (target->m_role == player_role::sheriff) {
                    return player_role::outlaw;
                }
            } else if (killer) {
                if (target->m_role == player_role::outlaw && killer->m_role == player_role::renegade) {
                    return player_role::renegade;
                } else if (target->m_role == player_role::renegade && killer->m_role == player_role::deputy) {
                    return player_role::deputy;
                } else if (target->m_role == player_role::deputy && killer->m_role == player_role::outlaw) {
                    return player_role::outlaw;
                }
            }
            return player_role::unknown;
        }();

        if (winner_role != player_role::unknown) {
            for (const auto &p : m_players) {
                add_public_update<game_update_type::player_show_role>(p.id, p.m_role);
            }
            add_log("LOG_GAME_OVER");
            add_public_update<game_update_type::game_over>(winner_role);
        } else if (m_playing == target) {
            target->end_of_turn();
        } else if (killer) {
            if (m_players.size() > 3) {
                switch (target->m_role) {
                case player_role::outlaw:
                    draw_card_to(card_pile_type::player_hand, killer);
                    draw_card_to(card_pile_type::player_hand, killer);
                    draw_card_to(card_pile_type::player_hand, killer);
                    break;
                case player_role::deputy:
                    if (killer->m_role == player_role::sheriff) {
                        killer->discard_all();
                    }
                    break;
                }
            } else {
                draw_card_to(card_pile_type::player_hand, killer);
                draw_card_to(card_pile_type::player_hand, killer);
                draw_card_to(card_pile_type::player_hand, killer);
            }
        }
    }

    void game::player_death(player *killer, player *target) {
        if (killer != m_playing) killer = nullptr;
        
        if (!characters_disabled(target)) {
            for (character *c : target->m_characters) {
                c->on_unequip(target);
            }
        }

        if (target->m_characters.size() > 1) {
            add_public_update<game_update_type::player_clear_characters>(target->id);
        }

        target->m_characters.resize(1);
        target->add_player_flags(player_flags::dead);
        target->m_hp = 0;

        if (!m_first_dead) m_first_dead = target;

        queue_event<event_type::on_player_death_priority>(killer, target);
        queue_event<event_type::delayed_action>([=, this]{
            instant_event<event_type::on_player_death>(killer, target);

            target->discard_all();
            target->add_gold(-target->m_gold);
            if (killer) {
                add_log("LOG_PLAYER_KILLED", killer, target);
            } else {
                add_log("LOG_PLAYER_DIED", target);
            }

            add_public_update<game_update_type::player_hp>(target->id, 0, true);
            add_public_update<game_update_type::player_show_role>(target->id, target->m_role);
        });
    }

    void game::disable_table_cards() {
        if (m_table_cards_disabled++ == 0) {
            for (auto &p : m_players) {
                if (!has_scenario(scenario_flags::lasso) && &p == m_playing) continue;
                for (auto *c : p.m_table) {
                    c->on_unequip(&p);
                }
            }
        }
    }

    void game::enable_table_cards() {
        if (--m_table_cards_disabled == 0) {
            for (auto &p : m_players) {
                if (!has_scenario(scenario_flags::lasso) && &p == m_playing) continue;
                for (auto *c : p.m_table) {
                    c->on_equip(&p);
                }
            }
        }
    }

    bool game::table_cards_disabled(player *p) const {
        return has_scenario(scenario_flags::lasso)
            || (m_table_cards_disabled > 0 && p != m_playing);
    }
    
    void game::disable_characters() {
        if (m_characters_disabled++ == 0) {
            for (auto &p : m_players) {
                if (!p.alive()) continue;
                if (!has_scenario(scenario_flags::hangover) && &p == m_playing) continue;
                for (auto *c : p.m_characters) {
                    c->on_unequip(&p);
                }
            }
        }
    }
    
    void game::enable_characters() {
        if (--m_characters_disabled == 0) {
            for (auto &p : m_players) {
                if (!p.alive()) continue;
                if (!has_scenario(scenario_flags::hangover) && &p == m_playing) continue;
                for (auto *c : p.m_characters) {
                    c->on_equip(&p);
                }
            }
        }
    }

    bool game::characters_disabled(player *p) const {
        return has_scenario(scenario_flags::hangover)
            || (m_characters_disabled > 0 && p != m_playing);
    }

    void game::handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args) {
        if (!m_requests.empty() && p == top_request().target()) {
            enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &req) {
                if constexpr (picking_request<E>) {
                    if (req.valid_pile(args.pile)) {
                        player *target_player = args.player_id ? get_player(args.player_id) : nullptr;
                        card *target_card = args.card_id ? find_card(args.card_id) : nullptr;
                        auto req_copy = req;
                        req_copy.on_pick(args.pile, target_player, target_card);
                    }
                }
            }, top_request());
        }
    }

    void game::handle_action(ACTION_TAG(play_card), player *p, const play_card_args &args) {
        if (m_requests.empty() && m_playing == p) {
            p->play_card(args);
        }
    }

    void game::handle_action(ACTION_TAG(respond_card), player *p, const play_card_args &args) {
        p->respond_card(args);
    }
}