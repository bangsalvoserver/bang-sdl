#include "game.h"

#include "card.h"
#include "common/requests.h"
#include "common/net_enums.h"

#include <array>

namespace banggame {

    using namespace enums::flag_operators;

    static card_info make_card_info(const card &c) {
        card_info info;
        info.expansion = c.expansion;
        info.id = c.id;
        info.name = c.name;
        info.image = c.image;
        info.playable_offturn = c.playable_offturn;
        for (const auto &value : c.effects) {
            info.targets.emplace_back(value.target(), value.args());
        }
        for (const auto &value : c.responses) {
            info.response_targets.emplace_back(value.target(), value.args());
        }
        for (const auto &value : c.equips) {
            info.equip_targets.emplace_back(value.target(), value.args());
        }
        return info;
    }

    void game::send_card_update(const deck_card &c, player *owner, show_card_flags flags) {
        show_card_update obj;
        obj.info = make_card_info(c);
        obj.suit = c.suit;
        obj.value = c.value;
        obj.color = c.color;
        obj.flags = flags;

        if (!owner) {
            add_public_update<game_update_type::show_card>(obj);
        } else {
            for (auto &p : m_players) {
                if (&p == owner) {
                    add_private_update<game_update_type::show_card>(&p, obj);
                } else {
                    add_private_update<game_update_type::hide_card>(&p, c.id, flags);
                }
            }
        }
    }

    void game::send_character_update(const character &c, int player_id, int index) {
        player_character_update obj;
        obj.info = make_card_info(c);
        obj.type = c.type;
        obj.max_hp = c.max_hp;
        obj.player_id = player_id;
        obj.index = index;
        
        add_public_update<game_update_type::player_add_character>(std::move(obj));
    }
    
    deck_card &game::move_to(deck_card &&c, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (known) {
            send_card_update(c, owner, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c.id, flags);
        }
        add_public_update<game_update_type::move_card>(c.id, 0, pile, flags);
        return [this, pile] () -> std::vector<deck_card>& {
            switch (pile) {
            case card_pile_type::main_deck:         return m_deck;
            case card_pile_type::discard_pile:      return m_discards;
            case card_pile_type::selection:         return m_selection;
            case card_pile_type::shop_selection:    return m_shop_selection;
            case card_pile_type::shop_discard:      return m_shop_discards;
            case card_pile_type::shop_hidden:       return m_shop_hidden;
            default: throw std::runtime_error("Pila non valida");
            }
        }().emplace_back(std::move(c));
    }

    void game::start_game(const game_options &options) {
        add_event<event_type::delayed_action>(0, [](std::function<void()> fun) { fun(); });
        
        std::random_device rd;
        rng.seed(rd());

        std::vector<character> characters;
        for (const auto &c : all_cards.characters) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.allowed_expansions)) {
                characters.emplace_back(c).id = get_next_id();
            }
        }

        std::ranges::shuffle(characters, rng);
        auto character_it = characters.begin();
        
        std::array roles {
            player_role::sheriff,
            player_role::outlaw,
            player_role::outlaw,
            player_role::renegade,
            player_role::deputy,
            player_role::outlaw,
            player_role::deputy,
            player_role::renegade
        };

        std::array roles_3players {
            player_role::deputy,
            player_role::outlaw,
            player_role::renegade
        };

        auto role_it = m_players.size() > 3 ? roles.begin() : roles_3players.begin();

#ifdef TESTING_CHARACTER
        auto testing_char = std::ranges::find(characters, TESTING_CHARACTER, &character::image);
        std::swap(*character_it, *testing_char);
#endif
        std::ranges::shuffle(role_it, role_it + options.nplayers, rng);
        for (auto &p : m_players) {
            p.set_character_and_role(std::move(*character_it++), *role_it++);
        }

        for (; character_it != characters.end(); ++character_it) {
            if (character_it->expansion == card_expansion_type::base) {
                m_base_characters.push_back(std::move(*character_it));
            }
        }

        for (const auto &c : all_cards.deck) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.allowed_expansions)) {
                m_deck.emplace_back(c).id = get_next_id();
            }
        }
        auto ids_view = m_deck | std::views::transform(&deck_card::id);
        add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::main_deck);
        shuffle_cards_and_ids(m_deck, rng);

        if (bool(options.allowed_expansions & card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                m_shop_deck.emplace_back(c).id = get_next_id();
            }
            ids_view = m_shop_deck | std::views::transform(&deck_card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_deck);
            shuffle_cards_and_ids(m_shop_deck, rng);

            for (const auto &c : all_cards.goldrush_choices) {
                m_shop_hidden.emplace_back(c).id = get_next_id();
            }
            ids_view = m_shop_hidden | std::views::transform(&deck_card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_hidden);
        }

        int max_initial_cards = std::ranges::max(m_players | std::views::transform(&player::get_initial_cards));
        for (int i=0; i<max_initial_cards; ++i) {
            for (auto &p : m_players) {
                if (p.m_hand.size() < p.get_initial_cards()) {
                    p.add_to_hand(draw_card());
                }
            }
        }

        if (!m_shop_deck.empty()) {
            for (int i=0; i<3; ++i) {
                move_to(draw_shop_card(), card_pile_type::shop_selection);
            }
        }

        if (m_players.size() > 3) {
            m_playing = &*std::ranges::find(m_players, player_role::sheriff, &player::m_role);
        } else {
            m_playing = &*std::ranges::find(m_players, player_role::deputy, &player::m_role);
        }

        queue_event<event_type::on_game_start>();
        m_playing->start_of_turn();
    }

    void game::tick() {
        if (!m_requests.empty()) {
            enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &obj) {
                if constexpr (timer_request<E>) {
                    if (obj.duration && --obj.duration == 0) {
                        if constexpr (requires { obj.on_finished(); }) {
                            auto copy = std::move(obj);
                            copy.on_finished();
                        } else {
                            pop_request();
                        }
                    }
                }
            }, top_request());
        }
    }

    deck_card game::draw_card() {
        if (m_deck.empty()) {
            deck_card top_discards = std::move(m_discards.back());
            m_discards.resize(m_discards.size()-1);
            m_deck = std::move(m_discards);
            m_discards.clear();
            m_discards.emplace_back(std::move(top_discards));
            shuffle_cards_and_ids(m_deck, rng);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::main_deck);
        }
        deck_card c = std::move(m_deck.back());
        m_deck.pop_back();
        return c;
    }

    deck_card game::draw_shop_card() {
        if (m_shop_deck.empty()) {
            m_shop_deck = std::move(m_shop_discards);
            m_shop_discards.clear();
            shuffle_cards_and_ids(m_shop_deck, rng);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::shop_deck);
        }
        deck_card c = std::move(m_shop_deck.back());
        m_shop_deck.pop_back();
        return c;
    }

    deck_card game::draw_from_discards() {
        deck_card c = std::move(m_discards.back());
        m_discards.pop_back();
        return c;
    }

    deck_card game::draw_from_temp(int card_id) {
        auto it = std::ranges::find(m_selection, card_id, &deck_card::id);
        if (it == m_selection.end()) throw game_error("server.draw_from_temp: ID non trovato");
        deck_card c = std::move(*it);
        m_selection.erase(it);
        return c;
    }

    void game::draw_check_then(player *p, draw_check_function fun, bool force_one, bool invert_pop_req) {
        if (force_one || p->m_num_checks == 1) {
            auto &moved = move_to(draw_card(), card_pile_type::discard_pile);
            auto suit = moved.suit;
            auto value = moved.value;
            queue_event<event_type::on_draw_check>(moved.id);
            fun(suit, value);
        } else {
            m_pending_checks.push_back(std::move(fun));
            for (int i=0; i<p->m_num_checks; ++i) {
                move_to(draw_card(), card_pile_type::selection);
            }
            add_request<request_type::check>(0, p, p).invert_pop_req = invert_pop_req;
        }
    }

    void game::resolve_check(int card_id) {
        auto c = draw_from_temp(card_id);
        for (auto &c : m_selection) {
            move_to(std::move(c), card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(c.id);
        }
        m_selection.clear();
        auto &moved = move_to(std::move(c), card_pile_type::discard_pile);
        auto suit = moved.suit;
        auto value = moved.value;
        queue_event<event_type::on_draw_check>(moved.id);
        m_pending_checks.front()(suit, value);
        m_pending_checks.pop_front();
    }

    void game::check_game_over(player *target, bool discarded_ghost) {
        player *killer = m_playing;

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
            } else if (!discarded_ghost) {
                if (target->m_role == player_role::outlaw && killer->m_role == player_role::deputy) {
                    return player_role::deputy;
                } else if (target->m_role == player_role::renegade && killer->m_role == player_role::outlaw) {
                    return player_role::outlaw;
                } else if (target->m_role == player_role::deputy && killer->m_role == player_role::renegade) {
                    return player_role::renegade;
                }
            }
            return player_role::unknown;
        }();

        if (winner_role != player_role::unknown) {
            for (const auto &p : m_players) {
                add_public_update<game_update_type::player_show_role>(p.id, p.m_role);
            }
            add_public_update<game_update_type::game_over>(winner_role);
        } else if (killer == target) {
            target->end_of_turn();
        } else if (!discarded_ghost) {
            if (m_players.size() > 3) {
                switch (target->m_role) {
                case player_role::outlaw:
                    killer->add_to_hand(target->m_game->draw_card());
                    killer->add_to_hand(target->m_game->draw_card());
                    killer->add_to_hand(target->m_game->draw_card());
                    break;
                case player_role::deputy:
                    if (killer->m_role == player_role::sheriff) {
                        killer->discard_all();
                    }
                    break;
                }
            } else {
                killer->add_to_hand(target->m_game->draw_card());
                killer->add_to_hand(target->m_game->draw_card());
                killer->add_to_hand(target->m_game->draw_card());
            }
        }
    }

    void game::player_death(player *target) {
        for (auto &c : target->m_characters) {
            c.on_unequip(target);
        }
        
        target->m_dead = true;
        target->m_hp = 0;
        
        instant_event<event_type::on_player_death>(m_playing, target);
        target->discard_all();

        add_public_update<game_update_type::player_hp>(target->id, 0, true);
        add_public_update<game_update_type::player_show_role>(target->id, target->m_role);

        check_game_over(target);
    }

    void game::handle_event(event_args &event) {
        for (auto &[card_id, e] : m_event_handlers) {
            if (e.index() == event.index()) {
                enums::visit_indexed([&]<event_type T>(enums::enum_constant<T>, auto &fun) {
                    std::apply(fun, std::get<enums::indexof(T)>(event));
                }, e);
            }
        }
    }

    void game::disable_table_cards(int player_id) {
        auto it = std::ranges::find(m_table_card_disablers, player_id);
        if (it == m_table_card_disablers.end()) {
            m_table_card_disablers.push_back(player_id);
            for (auto &p : m_players) {
                if (!p.alive() || p.id == player_id) continue;
                for (auto &c : p.m_table) {
                    c.on_unequip(&p);
                }
            }
        }
    }

    void game::enable_table_cards(int player_id) {
        auto it = std::ranges::find(m_table_card_disablers, player_id);
        if (it != m_table_card_disablers.end()) {
            m_table_card_disablers.erase(it);
            for (auto &p : m_players) {
                if (!p.alive() || p.id == player_id) continue;
                for (auto &c : p.m_table) {
                    c.on_equip(&p);
                }
            }
        }
    }

    bool game::table_cards_disabled(int player_id) {
        return !m_table_card_disablers.empty() && std::ranges::find(m_table_card_disablers, player_id) == m_table_card_disablers.end();
    }

    void game::handle_action(enums::enum_constant<game_action_type::pick_card>, player *p, const pick_card_args &args) {
        if (!m_requests.empty() && p == top_request().target()) {
            enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &req) {
                if constexpr (picking_request<E>) {
                    auto req_copy = req;
                    req_copy.on_pick(args);
                }
            }, top_request());
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::play_card>, player *p, const play_card_args &args) {
        if (m_requests.empty() && m_playing == p) {
            p->play_card(args);
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::respond_card>, player *p, const play_card_args &args) {
        p->respond_card(args);
    }

    void game::handle_action(enums::enum_constant<game_action_type::draw_from_deck>, player *p) {
        if (m_requests.empty() && m_playing == p && p->m_num_drawn_cards < p->m_num_cards_to_draw) {
            p->draw_from_deck();
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::pass_turn>, player *p) {
        if (m_requests.empty() && m_playing == p && p->m_num_drawn_cards >= p->m_num_cards_to_draw) {
            p->pass_turn();
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::resolve>, player *p) {
        if (!m_requests.empty() && p == top_request().target()) {
            enums::visit_indexed([]<request_type E>(enums::enum_constant<E>, auto &req) {
                if constexpr (resolvable_request<E>) {
                    auto req_copy = std::move(req);
                    req_copy.on_resolve();
                }
            }, top_request());
        }
    }
}