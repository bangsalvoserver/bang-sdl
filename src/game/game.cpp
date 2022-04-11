#include "game.h"

#include "holders.h"
#include "server/net_enums.h"

#include "effects/requests_armedanddangerous.h"
#include "effects/requests_base.h"

#include <array>

namespace banggame {

    using namespace enums::flag_operators;

    void game::send_card_update(const card &c, player *owner, show_card_flags flags) {
        if (!owner || bool(flags & show_card_flags::show_everyone)) {
            add_public_update<game_update_type::show_card>(c, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c.id, flags, owner->id);
            add_private_update<game_update_type::show_card>(owner, c, flags);
        }
    }

    card *game::find_card(int card_id) {
        if (auto it = m_cards.find(card_id); it != m_cards.end()) {
            return &*it;
        }
        throw game_error("server.find_card: ID not found"_nonloc);
    }

    player *game::find_player(int player_id) {
        if (auto it = m_players.find(player_id); it != m_players.end()) {
            return &*it;
        }
        throw game_error("server.find_player: ID not found"_nonloc);
    }

    player *game::find_disconnected_player() {
        auto dc_view = std::ranges::filter_view(m_players, [](const player &p) { return p.client_id == 0; });
        if (dc_view.empty()) return nullptr;

        auto first_alive = std::ranges::find_if(dc_view, &player::alive);
        if (first_alive != dc_view.end()) {
            return &*first_alive;
        }
        if (has_expansion(card_expansion_type::ghostcards)) {
            return &dc_view.front();
        } else {
            return nullptr;
        }
    }

    std::vector<game_update> game::get_game_state_updates(player *owner) {
        std::vector<game_update> ret;
#define ADD_TO_RET(name, ...) ret.emplace_back(enums::enum_tag<game_update_type::name> __VA_OPT__(,) __VA_ARGS__)
        
        ADD_TO_RET(add_cards, make_id_vector(m_cards | std::views::transform([](const card &c) { return &c; })), card_pile_type::hidden_deck);

        const auto show_never = [](const card &c) { return false; };
        const auto show_always = [](const card &c) { return true; };

        auto move_cards = [&](auto &&range, auto do_show_card) {
            for (card *c : range) {
                ADD_TO_RET(move_card, c->id, c->owner ? c->owner->id : 0, c->pile, show_card_flags::no_animation);

                if (do_show_card(*c)) {
                    ADD_TO_RET(show_card, *c, show_card_flags::no_animation);

                    for (int id : c->cubes) {
                        ADD_TO_RET(move_cube, id, c->id);
                    }

                    if (c->inactive) {
                        ADD_TO_RET(tap_card, c->id, true, true);
                    }
                }
            }
        };

        move_cards(m_specials, show_always);
        move_cards(m_deck, show_never);
        move_cards(m_shop_deck, show_never);

        move_cards(m_discards, show_always);
        move_cards(m_selection, [&](const card &c){ return c.owner == owner; });
        move_cards(m_shop_discards, show_always);
        move_cards(m_shop_selection, show_always);
        move_cards(m_hidden_deck, show_always);

        move_cards(m_scenario_deck, [&](const card &c) { return &c == m_scenario_deck.back(); });
        move_cards(m_scenario_cards, [&](const card &c) { return &c == m_scenario_cards.back(); });
        
        if (!m_cubes.empty()) {
            ADD_TO_RET(add_cubes, m_cubes);
        }

        for (auto &p : m_players) {
            if (p.check_player_flags(player_flags::role_revealed) || &p == owner) {
                ADD_TO_RET(player_show_role, p.id, p.m_role, true);
            }

            if (p.alive() || has_expansion(card_expansion_type::ghostcards)) {
                move_cards(p.m_characters, show_always);
                move_cards(p.m_backup_character, show_never);

                move_cards(p.m_table, show_always);
                move_cards(p.m_hand, [&](const card &c){ return c.owner == owner || c.deck == card_deck_type::character; });

                ADD_TO_RET(player_hp, p.id, p.m_hp, !p.alive(), true);
                
                if (p.m_gold != 0) {
                    ADD_TO_RET(player_gold, p.id, p.m_gold);
                }
            }
        }

        if (m_playing) {
            ADD_TO_RET(switch_turn, m_playing->id);
        }
        if (!m_requests.empty()) {
            ADD_TO_RET(request_status, make_request_update(owner));
        }
        if (owner && owner->m_forced_card) {
            ADD_TO_RET(force_play_card, owner->m_forced_card->id);
        }
        for (const auto &str : m_saved_log) {
            ADD_TO_RET(game_log, str);
        }

#undef ADD_TO_RET
        return ret;
    }

    void game::shuffle_cards_and_ids(std::vector<card *> &vec) {
        for (size_t i = vec.size() - 1; i > 0; --i) {
            size_t i2 = std::uniform_int_distribution<size_t>{0, i}(rng);
            if (i == i2) continue;

            std::swap(vec[i], vec[i2]);
            auto a = m_cards.extract(vec[i]->id);
            auto b = m_cards.extract(vec[i2]->id);
            std::swap(a->id, b->id);
            m_cards.insert(std::move(a));
            m_cards.insert(std::move(b));
        }
    }

    void game::start_game(const game_options &options, const all_cards_t &all_cards) {
        m_options = options;

#ifndef NDEBUG
        std::vector<card *> testing_cards;
#endif
        
        auto add_card = [&](card_pile_type pile, const card_deck_info &c) {
            card copy(c);
            copy.id = m_cards.first_available_id();
            copy.owner = nullptr;
            copy.pile = pile;
            auto *new_card = &m_cards.emplace(std::move(copy));

#ifndef NDEBUG
            if (c.testing) {
                testing_cards.push_back(new_card);
            } else {
                get_pile(pile).push_back(new_card);
            }
#else
            get_pile(pile).push_back(new_card);
#endif
        };

        for (const auto &c : all_cards.specials) {
            if ((c.expansion & m_options.expansions) == c.expansion) {
                add_card(card_pile_type::specials, c);
            }
        }
        add_public_update<game_update_type::add_cards>(make_id_vector(m_specials), card_pile_type::specials);
        for (const auto &c : m_specials) {
            send_card_update(*c, nullptr, show_card_flags::no_animation);
        }

        for (const auto &c : all_cards.deck) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if ((c.expansion & m_options.expansions) == c.expansion) {
                add_card(card_pile_type::main_deck, c);
            }
        }

        shuffle_cards_and_ids(m_deck);
#ifndef NDEBUG
        m_deck.insert(m_deck.end(), testing_cards.begin(), testing_cards.end());
        testing_cards.clear();
#endif
        add_public_update<game_update_type::add_cards>(make_id_vector(m_deck), card_pile_type::main_deck);

        if (has_expansion(card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                add_card(card_pile_type::shop_deck, c);
            }
            shuffle_cards_and_ids(m_shop_deck);
#ifndef NDEBUG
            m_shop_deck.insert(m_shop_deck.end(), testing_cards.begin(), testing_cards.end());
            testing_cards.clear();
#endif
            add_public_update<game_update_type::add_cards>(make_id_vector(m_shop_deck), card_pile_type::shop_deck);
        }

        if (has_expansion(card_expansion_type::armedanddangerous)) {
            auto cube_ids = std::views::iota(1, 32);
            m_cubes.assign(cube_ids.begin(), cube_ids.end());
            add_public_update<game_update_type::add_cubes>(m_cubes);
        }

        std::vector<card *> last_scenario_cards;

        if (has_expansion(card_expansion_type::highnoon)) {
            for (const auto &c : all_cards.highnoon) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                if ((c.expansion & m_options.expansions) == c.expansion) {
                    add_card(card_pile_type::scenario_deck, c);
                }
            }
            last_scenario_cards.push_back(m_scenario_deck.back());
            m_scenario_deck.pop_back();
        }
        
        if (has_expansion(card_expansion_type::fistfulofcards)) {
            for (const auto &c : all_cards.fistfulofcards) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                if ((c.expansion & m_options.expansions) == c.expansion) {
                    add_card(card_pile_type::scenario_deck, c);
                }
            }
            last_scenario_cards.push_back(m_scenario_deck.back());
            m_scenario_deck.pop_back();
        }

        if (!m_scenario_deck.empty()) {
            shuffle_cards_and_ids(m_scenario_deck);
#ifndef NDEBUG
            m_scenario_deck.insert(m_scenario_deck.end(), testing_cards.begin(), testing_cards.end());
            testing_cards.clear();
#endif
            if (m_scenario_deck.size() > 12) {
                m_scenario_deck.resize(12);
            }
            m_scenario_deck.push_back(last_scenario_cards[std::uniform_int_distribution<>(0, last_scenario_cards.size() - 1)(rng)]);
            std::swap(m_scenario_deck.back(), m_scenario_deck.front());

            add_public_update<game_update_type::add_cards>(make_id_vector(m_scenario_deck), card_pile_type::scenario_deck);

            send_card_update(*m_scenario_deck.back(), nullptr, show_card_flags::no_animation);
        }

        for (const auto &c : all_cards.hidden) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if ((c.expansion & m_options.expansions) == c.expansion) {
                add_card(card_pile_type::hidden_deck, c);
            }
        }

        if (!m_hidden_deck.empty()) {
            add_public_update<game_update_type::add_cards>(make_id_vector(m_hidden_deck), card_pile_type::hidden_deck);
        }
        
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
            player_role::deputy_3p,
            player_role::outlaw_3p,
            player_role::renegade_3p
        };

        auto role_it = m_players.size() > 3 ? roles.begin() : roles_3players.begin();

        std::ranges::shuffle(role_it, role_it + m_players.size(), rng);
        for (auto &p : m_players) {
            p.set_role(*role_it++);
        }

        if (m_players.size() > 3) {
            m_first_player = &*std::ranges::find(m_players, player_role::sheriff, &player::m_role);
        } else {
            m_first_player = &*std::ranges::find(m_players, player_role::deputy_3p, &player::m_role);
        }

        add_log("LOG_GAME_START");

        std::vector<card *> character_ptrs;
        for (const auto &c : all_cards.characters) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.expansions)) {
                card copy(c);
                copy.id = m_cards.first_available_id();
                auto *new_card = &m_cards.emplace(std::move(copy));
#ifndef NDEBUG
                if (c.testing) {
                    testing_cards.push_back(new_card);
                } else {
                    character_ptrs.push_back(new_card);
                }
#else
                character_ptrs.push_back(new_card);
#endif
            }
        }

        std::ranges::shuffle(character_ptrs, rng);
#ifndef NDEBUG
        character_ptrs.insert(character_ptrs.begin(), testing_cards.begin(), testing_cards.end());
        testing_cards.clear();
#endif

        auto add_character_to = [&](card *c, player &p) {
            p.m_characters.push_back(c);
            c->pile = card_pile_type::player_character;
            c->owner = &p;
            add_public_update<game_update_type::add_cards>(make_id_vector(std::views::single(c)), card_pile_type::player_character, p.id);
        };

        auto character_it = character_ptrs.begin();

#ifdef NDEBUG
        for (auto &p : m_players) {
            add_character_to(*character_it++, p);
            add_character_to(*character_it++, p);
        }
#else
        for (auto &p : m_players) {
            add_character_to(*character_it++, p);
        }
        for (auto &p : m_players) {
            add_character_to(*character_it++, p);
        }
#endif

        if (has_expansion(card_expansion_type::characterchoice)) {
            for (auto &p : m_players) {
                while (!p.m_characters.empty()) {
                    move_to(p.m_characters.front(), card_pile_type::player_hand, true, &p, show_card_flags::no_animation | show_card_flags::show_everyone);
                }
            }
            auto *p = m_first_player;
            while (true) {
                queue_request<request_characterchoice>(p);

                p = get_next_player(p);
                if (p == m_first_player) break;
            }
        } else {
            for (auto &p : m_players) {
                send_card_update(*p.m_characters.front(), &p, show_card_flags::no_animation | show_card_flags::show_everyone);
                p.equip_if_enabled(p.m_characters.front());
                p.m_hp = p.m_max_hp;
                add_public_update<game_update_type::player_hp>(p.id, p.m_hp, false, true);

                move_to(p.m_characters.back(), card_pile_type::player_backup, false, &p, show_card_flags::no_animation);
            }
        }

        queue_action([this] {
            for (auto &p : m_players) {
                card *c = p.m_characters.front();
                for (auto &e : c->equips) {
                    e.on_pre_equip(c, &p);
                }
            }

            int max_initial_cards = std::ranges::max(m_players | std::views::transform(&player::get_initial_cards));
            for (int i=0; i<max_initial_cards; ++i) {
                for (auto &p : m_players) {
                    if (p.m_hand.size() < p.get_initial_cards()) {
                        draw_card_to(card_pile_type::player_hand, &p);
                    }
                }
            }

            if (!m_shop_deck.empty()) {
                for (int i=0; i<3; ++i) {
                    draw_shop_card();
                }
            }

            m_playing = m_first_player;
            m_first_player->start_of_turn();
        });
    }

    void game::tick() {
        if (!m_requests.empty()) {
            top_request().tick();
        }
    }

    request_status_args game::make_request_update(player *p) {
        const auto &req = top_request();
        request_status_args ret{
            req.origin() ? req.origin()->id : 0,
            req.target() ? req.target()->id : 0,
            req.status_text(p)
        };

        if (!p) return ret;

        auto add_ids_for = [&](auto &&cards) {
            for (card *c : cards) {
                if (p->can_respond_with(c)) {
                    ret.respond_ids.push_back(c->id);
                }
            }
        };

        add_ids_for(p->m_hand | std::views::filter([](card *c) { return c->color == card_color_type::brown; }));
        add_ids_for(p->m_table | std::views::filter(std::not_fn(&card::inactive)));
        add_ids_for(p->m_characters);
        add_ids_for(m_scenario_cards | std::views::reverse | std::views::take(1));
        add_ids_for(m_specials);

        if (req.target() != p) return ret;

        auto maybe_add_pick_id = [&](card_pile_type pile, player *target_player, card *target_card) {
            if (req.can_pick(pile, target_player, target_card)) {
                ret.pick_ids.emplace_back(pile,
                    target_player ? target_player->id : 0,
                    target_card ? target_card->id : 0);
            }
        };

        for (player &target : m_players) {
            std::ranges::for_each(target.m_hand, std::bind_front(maybe_add_pick_id, card_pile_type::player_hand, &target));
            std::ranges::for_each(target.m_table, std::bind_front(maybe_add_pick_id, card_pile_type::player_table, &target));
            std::ranges::for_each(target.m_characters, std::bind_front(maybe_add_pick_id, card_pile_type::player_character, &target));
        }
        maybe_add_pick_id(card_pile_type::main_deck, nullptr, nullptr);
        maybe_add_pick_id(card_pile_type::discard_pile, nullptr, nullptr);
        std::ranges::for_each(m_selection, std::bind_front(maybe_add_pick_id, card_pile_type::selection, nullptr));

        return ret;
    }

    void game::update_request() {
        if (m_requests.empty()) {
            add_public_update<game_update_type::status_clear>();
            while (m_requests.empty() && !m_delayed_actions.empty()) {
                std::invoke(m_delayed_actions.front());
                m_delayed_actions.pop_front();
            }
        } else {
            add_public_update<game_update_type::request_status>(make_request_update(nullptr));
            for (auto &p : m_players) {
                add_private_update<game_update_type::request_status>(&p, make_request_update(&p));
            }
        }
    }

    void game::queue_request_front(std::shared_ptr<request_base> &&value) {
        m_requests.emplace_front(std::move(value));
        update_request();
    }

    void game::queue_request(std::shared_ptr<request_base> &&value) {
        m_requests.emplace_back(std::move(value));
        if (m_requests.size() == 1) {
            update_request();
        }
    }

    std::vector<card *> &game::get_pile(card_pile_type pile, player *owner) {
        switch (pile) {
        case card_pile_type::player_hand:       return owner->m_hand;
        case card_pile_type::player_table:      return owner->m_table;
        case card_pile_type::player_character:  return owner->m_characters;
        case card_pile_type::player_backup:     return owner->m_backup_character;
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
        auto &prev_pile = get_pile(c->pile, c->owner);
        auto card_it = std::ranges::find(prev_pile, c);
        if (c->pile == pile && c->owner == owner) {
            return std::next(card_it);
        }
        add_public_update<game_update_type::move_card>(c->id, owner ? owner->id : 0, pile, flags);
        get_pile(pile, owner).emplace_back(c);
        c->pile = pile;
        c->owner = owner;
        return prev_pile.erase(card_it);
    }

    card *game::draw_card_to(card_pile_type pile, player *owner, show_card_flags flags) {
        card *drawn_card = m_deck.back();
        move_to(drawn_card, pile, true, owner, flags);
        if (m_deck.empty()) {
            card *top_discards = m_discards.back();
            m_discards.pop_back();
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
        if (drawn_card->modifier == card_modifier_type::shopchoice) {
            for (card *c : m_hidden_deck) {
                if (!c->effects.empty() && c->effects.front().type == drawn_card->effects.front().type) {
                    send_card_update(*c, nullptr, show_card_flags::no_animation);
                }
            }
        }
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
            m_first_player->unequip_if_enabled(m_scenario_cards.back());
            m_scenario_flags = {};
        }
        move_to(m_scenario_deck.back(), card_pile_type::scenario_card);
        m_first_player->equip_if_enabled(m_scenario_cards.back());
    }
    
    void game::draw_check_then(player *origin, card *origin_card, draw_check_function fun) {
        m_current_check.emplace(std::move(fun), origin, origin_card);
        do_draw_check();
    }

    void game::do_draw_check() {
        if (m_current_check->origin->m_num_checks == 1) {
            auto *c = draw_card_to(card_pile_type::discard_pile);
            add_log("LOG_CHECK_DREW_CARD", m_current_check->origin_card, m_current_check->origin, c);
            call_event<event_type::on_draw_check>(m_current_check->origin, c);
            if (!num_queued_requests([&]{
                call_event<event_type::on_draw_check_select>(m_current_check->origin_card, c);
            })) {
                m_current_check->function(c);
                m_current_check.reset();
            }
        } else {
            for (int i=0; i<m_current_check->origin->m_num_checks; ++i) {
                draw_card_to(card_pile_type::selection);
            }
            queue_request_front<request_check>(m_current_check->origin_card, m_current_check->origin);
        }
    }

    void game::queue_action(std::function<void()> &&fun) {
        if (m_requests.empty() && m_delayed_actions.empty()) {
            fun();
        } else {
            m_delayed_actions.push_back(std::move(fun));
        }
    }

    player *game::get_next_player(player *p) {
        auto it = m_players.find(p->id);
        do {
            if (++it == m_players.end()) it = m_players.begin();
        } while(!it->alive());
        return &*it;
    }

    player *game::get_next_in_turn(player *p) {
        auto it = m_players.find(p->id);
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
        if (from->check_player_flags(player_flags::disable_player_distances)) return to->m_distance_mod;
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
                if (target->m_role == player_role::outlaw_3p && killer->m_role == player_role::renegade_3p) {
                    return player_role::renegade;
                } else if (target->m_role == player_role::renegade_3p && killer->m_role == player_role::deputy_3p) {
                    return player_role::deputy;
                } else if (target->m_role == player_role::deputy_3p && killer->m_role == player_role::outlaw_3p) {
                    return player_role::outlaw;
                }
            }
            return player_role::unknown;
        }();

        if (winner_role != player_role::unknown) {
            add_public_update<game_update_type::status_clear>();
            for (const auto &p : m_players) {
                if (!p.check_player_flags(player_flags::role_revealed)) {
                    add_public_update<game_update_type::player_show_role>(p.id, p.m_role);
                }
            }
            add_log("LOG_GAME_OVER");
            add_public_update<game_update_type::game_over>(winner_role);
            m_game_over = true;
        } else if (m_playing == target) {
            get_next_in_turn(target)->start_of_turn();
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
                        queue_action([killer]{
                            killer->discard_all();
                        });
                    }
                    break;
                }
            } else {
                draw_card_to(card_pile_type::player_hand, killer);
                draw_card_to(card_pile_type::player_hand, killer);
                draw_card_to(card_pile_type::player_hand, killer);
            }
        }

        if (!has_expansion(card_expansion_type::ghostcards)) {
            add_public_update<game_update_type::player_remove>(target->id);
        }
    }

    void game::player_death(player *killer, player *target) {
        if (killer != m_playing) killer = nullptr;
        
        for (card *c : target->m_characters) {
            target->unequip_if_enabled(c);
        }

        if (target->m_characters.size() > 1) {
            add_public_update<game_update_type::remove_cards>(make_id_vector(target->m_characters | std::views::drop(1)));
            target->m_characters.resize(1);
        }

        target->add_player_flags(player_flags::dead);
        target->m_hp = 0;

        if (!m_first_dead) m_first_dead = target;

        call_event<event_type::on_player_death>(killer, target);

        target->discard_all();
        target->add_gold(-target->m_gold);
        if (killer) {
            add_log("LOG_PLAYER_KILLED", killer, target);
        } else {
            add_log("LOG_PLAYER_DIED", target);
        }

        add_public_update<game_update_type::player_hp>(target->id, 0, true);
        add_public_update<game_update_type::player_show_role>(target->id, target->m_role);
        target->add_player_flags(player_flags::role_revealed);
    }

    void game::add_disabler(event_card_key key, card_disabler_fun &&fun) {
        const auto disable_if_new = [&](card *c) {
            if (!is_disabled(c) && fun(c)) {
                for (auto &e : c->equips) {
                    e.on_unequip(c, c->owner);
                }
            }
        };

        for (auto &p : m_players) {
            std::ranges::for_each(p.m_table, disable_if_new);
            std::ranges::for_each(p.m_characters, disable_if_new);
        }

        m_disablers.add(key, std::move(fun));
    }

    void game::remove_disablers(event_card_key key) {
        const auto enable_if_old = [&](card *c) {
            bool a = false;
            bool b = false;
            for (const auto &[t, fun] : m_disablers) {
                if (t != key) a = a || fun(c);
                else b = b || fun(c);
            }
            if (!a && b) {
                for (auto &e : c->equips) {
                    e.on_equip(c, c->owner);
                }
            }
        };

        for (auto &p : m_players) {
            std::ranges::for_each(p.m_table, enable_if_old);
            std::ranges::for_each(p.m_characters, enable_if_old);
        }

        m_disablers.erase(key);
    }

    bool game::is_disabled(card *target_card) const {
        for (const auto &fun : m_disablers | std::views::values) {
            if (fun(target_card)) return true;
        }
        return false;
    }

    void game::handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args) {
        if (m_requests.empty()) return;
        if (auto &req = top_request(); req.target() == p) {
            add_private_update<game_update_type::confirm_play>(p);
            player *target_player = args.player_id ? find_player(args.player_id) : nullptr;
            card *target_card = args.card_id ? find_card(args.card_id) : nullptr;
            if (req.can_pick(args.pile, target_player, target_card)) {
                req.on_pick(args.pile, target_player, target_card);
            }
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

    void game::handle_action(ACTION_TAG(prompt_respond), player *p, bool response) {
        p->prompt_response(response);
    }
}