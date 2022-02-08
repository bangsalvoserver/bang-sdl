#include "game.h"

#include "common/holders.h"
#include "common/net_enums.h"

#include <array>

namespace banggame {

    using namespace enums::flag_operators;
    using namespace std::placeholders;

    static card_info make_card_info(const card &c) {
        auto make_card_info_effects = []<typename T>(std::vector<T> &out, const auto &vec) {
            for (const auto &value : vec) {
                T obj;
                obj.type = value.type;
                obj.target = value.target;
                obj.args = value.args;
                out.push_back(std::move(obj));
            }
        };
        
        card_info info;
        info.expansion = c.expansion;
        info.id = c.id;
        info.name = c.name;
        info.image = c.image;
        info.modifier = c.modifier;
        make_card_info_effects(info.targets, c.effects);
        make_card_info_effects(info.response_targets, c.responses);
        make_card_info_effects(info.optional_targets, c.optionals);
        make_card_info_effects(info.equip_targets, c.equips);
        return info;
    }

    void game::send_card_update(const card &c, player *owner, show_card_flags flags) {
        show_card_update obj;
        obj.info = make_card_info(c);
        obj.suit = c.suit;
        obj.value = c.value;
        obj.color = c.color;
        obj.flags = flags;

        if (!owner || bool(flags & show_card_flags::show_everyone)) {
            add_public_update<game_update_type::show_card>(obj);
        } else {
            add_public_update<game_update_type::hide_card>(c.id, flags, owner->id);
            add_private_update<game_update_type::show_card>(owner, obj);
        }
    }

    card *game::find_card(int card_id) {
        if (auto it = m_cards.find(card_id); it != m_cards.end()) {
            return &it->second;
        } else if (auto it = m_characters.find(card_id); it != m_characters.end()) {
            return &it->second;
        }
        throw game_error("server.find_card: ID not found"_nonloc);
    }

    std::vector<game_update> game::get_game_state_updates(player *owner) {
        std::vector<game_update> ret;

        auto add_cards = [&](auto fun, card_pile_type pile) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{},
                make_id_vector(m_cards
                    | std::views::values
                    | std::views::filter([&](const card &c) {
                        return fun(c.expansion);
                    })),
                pile);
        };

        auto show_card = [&](card *c) {
            show_card_update obj;
            obj.info = make_card_info(*c);
            obj.suit = c->suit;
            obj.value = c->value;
            obj.color = c->color;
            obj.flags = show_card_flags::no_animation;

            ret.emplace_back(enums::enum_constant<game_update_type::show_card>{}, std::move(obj));
        };

        auto move_cards = [&](std::vector<card *> &vec, bool known = true) {
            for (card *c : vec) {
                ret.emplace_back(enums::enum_constant<game_update_type::move_card>{},
                    c->id, c->owner ? c->owner->id : 0, c->pile, show_card_flags::no_animation);

                if (known || c->owner == owner) {
                    show_card(c);

                    for (int id : c->cubes) {
                        ret.emplace_back(enums::enum_constant<game_update_type::move_cube>{}, id, c->id);
                    }

                    if (c->inactive) {
                        ret.emplace_back(enums::enum_constant<game_update_type::tap_card>{}, c->id, true, true);
                    }
                }
            }
        };

        add_cards([](card_expansion_type expansion) {
            return !bool(expansion &
                ( card_expansion_type::goldrush
                | card_expansion_type::highnoon
                | card_expansion_type::fistfulofcards ));
        }, card_pile_type::main_deck);

        add_cards([](card_expansion_type expansion) {
            return bool(expansion &
                card_expansion_type::goldrush);
        }, card_pile_type::shop_deck);

        ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, make_id_vector(m_specials), card_pile_type::specials);
        std::ranges::for_each(m_specials, show_card);

        move_cards(m_discards);
        move_cards(m_selection, false);
        move_cards(m_shop_discards);
        move_cards(m_shop_selection);
        move_cards(m_hidden_deck);

        if (!m_scenario_deck.empty()) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, make_id_vector(m_scenario_deck),
                card_pile_type::scenario_deck);
            show_card(m_scenario_deck.back());
        }
        if (!m_scenario_cards.empty()) {
            card *c = m_scenario_cards.back();
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, std::vector{c->id},
                card_pile_type::scenario_deck);
            ret.emplace_back(enums::enum_constant<game_update_type::move_card>{},
                c->id, 0, card_pile_type::scenario_card, show_card_flags::no_animation);
            show_card(c);
        }
        
        if (!m_cubes.empty()) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cubes>{}, m_cubes);
        }

        for (auto &p : m_players) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, make_id_vector(p.m_characters), card_pile_type::player_character, p.id);
            std::ranges::for_each(p.m_characters, show_card);

            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, make_id_vector(p.m_backup_character), card_pile_type::player_backup, p.id);

            ret.emplace_back(enums::enum_constant<game_update_type::player_hp>{}, p.id, p.m_hp, !p.alive(), true);
            
            if (p.m_gold != 0) {
                ret.emplace_back(enums::enum_constant<game_update_type::player_gold>{}, p.m_gold);
            }

            if (p.m_role == player_role::sheriff || p.m_hp == 0 || m_players.size() < 4) {
                ret.emplace_back(enums::enum_constant<game_update_type::player_show_role>{}, p.id, p.m_role, true);
            }

            move_cards(p.m_table);
            move_cards(p.m_hand, false);
        }

        ret.emplace_back(enums::enum_constant<game_update_type::switch_turn>{}, m_playing->id);
        if (!m_requests.empty()) {
            auto &req = top_request();
            ret.emplace_back(enums::enum_constant<game_update_type::request_status>{},
                req.enum_index(),
                req.origin() ? req.origin()->id : 0,
                req.target() ? req.target()->id : 0,
                req.flags(),
                req.status_text());
            ret.emplace_back(enums::enum_constant<game_update_type::request_respond>{}, make_request_respond(owner));
        }

        return ret;
    }

    void game::shuffle_cards_and_ids(std::vector<card *> &vec) {
        for (size_t i = vec.size() - 1; i > 0; --i) {
            card *a = vec[i];
            card *b = vec[std::uniform_int_distribution<>{0, int(i)}(rng)];
            std::swap(*a, *b);
            std::swap(a->id, b->id);
        }
    }

struct to_begin{};
struct to_end{};

#ifndef DISABLE_TESTING
    static auto get_iterator(to_begin, auto &vec) {
        return vec.begin();
    }

    static auto get_iterator(to_end, auto &vec) {
        return vec.rbegin();
    }

    template<typename Tag, std::derived_from<card> T>
    static void swap_testing(std::vector<T *> &vec) {
        auto pos = get_iterator(Tag{}, vec);
        for (auto &ptr : vec | std::views::filter(&T::testing)) {
            std::swap(ptr, *pos++);
        }
    }
#else
    template<typename Tag, std::derived_from<card> T>
    static void swap_testing(std::vector<T *> &vec) {}
#endif

    void game::start_game(const game_options &options, const all_cards_t &all_cards) {
        m_options = options;
        
        add_event<event_type::delayed_action>(0, [](std::function<void()> fun) { fun(); });

        std::vector<character *> character_ptrs;
        for (const auto &c : all_cards.characters) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.expansions)) {
                auto it = m_characters.emplace(get_next_id(), c).first;
                character_ptrs.emplace_back(&it->second)->id = it->first;
            }
        }

        std::ranges::shuffle(character_ptrs, rng);
        swap_testing<to_begin>(character_ptrs);

        auto character_it = character_ptrs.begin();
        
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

        std::ranges::shuffle(role_it, role_it + m_players.size(), rng);
        for (auto &p : m_players) {
            p.set_character_and_role(*character_it++, *role_it++);
        }
        for (auto &p : m_players) {
            p.set_backup_character(*character_it++);
        }

        for (; character_it != character_ptrs.end(); ++character_it) {
            if ((*character_it)->expansion == card_expansion_type::base) {
                m_base_characters.push_back(*character_it);
            }
        }

        auto add_card = [&](card_pile_type pile, const card &c) {
            auto it = m_cards.emplace(get_next_id(), c).first;
            auto *new_card = get_pile(pile).emplace_back(&it->second);
            new_card->id = it->first;
            new_card->owner = nullptr;
            new_card->pile = pile;
        };

        for (const auto &c : all_cards.specials) {
            if ((c.expansion & options.expansions) == c.expansion) {
                add_card(card_pile_type::specials, c);
            }
        }
        add_public_update<game_update_type::add_cards>(make_id_vector(m_specials), card_pile_type::specials);
        for (const auto &c : m_specials) {
            send_card_update(*c, nullptr, show_card_flags::no_animation);
        }

        for (const auto &c : all_cards.deck) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if ((c.expansion & options.expansions) == c.expansion) {
                add_card(card_pile_type::main_deck, c);
            }
        }

        add_public_update<game_update_type::add_cards>(make_id_vector(m_deck), card_pile_type::main_deck);
        shuffle_cards_and_ids(m_deck);
        swap_testing<to_end>(m_deck);

        if (has_expansion(card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                add_card(card_pile_type::shop_deck, c);
            }
            add_public_update<game_update_type::add_cards>(make_id_vector(m_shop_deck), card_pile_type::shop_deck);
            shuffle_cards_and_ids(m_shop_deck);
            swap_testing<to_end>(m_shop_deck);
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
            if (m_scenario_deck.size() > 12) {
                m_scenario_deck.resize(12);
            }
            m_scenario_deck.push_back(last_scenario_cards[std::uniform_int_distribution<>(0, last_scenario_cards.size() - 1)(rng)]);
            std::swap(m_scenario_deck.back(), m_scenario_deck.front());

            swap_testing<to_end>(m_scenario_deck);

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

        if (m_players.size() > 3) {
            m_playing = &*std::ranges::find(m_players, player_role::sheriff, &player::m_role);
        } else {
            m_playing = &*std::ranges::find(m_players, player_role::deputy, &player::m_role);
        }

        for (auto &p : m_players) {
            card *c = p.m_characters.front();
            for (auto &e : c->equips) {
                e.on_pre_equip(c, &p);
            }
        }
        add_log("LOG_GAME_START");
        m_first_player = m_playing;
        m_first_player->start_of_turn();
    }

    void game::tick() {
        if (!m_requests.empty() && top_request().tick()) {
            pop_request();
        }
    }

    request_respond_args game::make_request_respond(player *p) {
        request_respond_args ret;

        auto add_ids_for = [&](auto &&cards) {
            for (card *c : cards) {
                if (!is_disabled(c) && std::ranges::any_of(c->responses, [&](const effect_holder &e) {
                    return e.can_respond(c, p);
                })) ret.respond_ids.push_back(c->id);
            }
        };

        add_ids_for(p->m_hand | std::views::filter([](card *c) { return c->color == card_color_type::brown; }));
        add_ids_for(p->m_table | std::views::filter(std::not_fn(&card::inactive)));
        add_ids_for(p->m_characters);
        add_ids_for(m_scenario_cards | std::views::reverse | std::views::take(1));
        add_ids_for(m_specials);

        auto maybe_add_pick_id = [&](card_pile_type pile, player *target_player, card *target_card) {
            if (top_request().can_pick(pile, target_player, target_card)) {
                ret.pick_ids.emplace_back(pile,
                    target_player ? target_player->id : 0,
                    target_card ? target_card->id : 0);
            }
        };

        for (player &target : m_players) {
            maybe_add_pick_id(card_pile_type::player, &target, nullptr);
            std::ranges::for_each(target.m_hand, std::bind(maybe_add_pick_id, card_pile_type::player_hand, &target, _1));
            std::ranges::for_each(target.m_table, std::bind(maybe_add_pick_id, card_pile_type::player_table, &target, _1));
            std::ranges::for_each(target.m_characters, std::bind(maybe_add_pick_id, card_pile_type::player_character, &target, _1));
        }
        maybe_add_pick_id(card_pile_type::main_deck, nullptr, nullptr);
        maybe_add_pick_id(card_pile_type::discard_pile, nullptr, nullptr);
        std::ranges::for_each(m_selection, std::bind(maybe_add_pick_id, card_pile_type::selection, nullptr, _1));

        return ret;
    }

    void game::send_request_respond() {
        if (player *target = top_request().target()) {
            add_private_update<game_update_type::request_respond>(target, make_request_respond(target));
        } else {
            for (auto &p : m_players) {
                add_private_update<game_update_type::request_respond>(&p, make_request_respond(&p));
            }
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
            m_first_player->unequip_if_enabled(m_scenario_cards.back());
            m_scenario_flags = enums::flags_none<scenario_flags>;
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

    bool game::pop_request_noupdate(request_type type) {
        if (type != request_type::none && !top_request_is(type)) return false;
        m_requests.front().cleanup();
        m_requests.pop_front();
        return true;
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
        
        for (card *c : target->m_characters) {
            target->unequip_if_enabled(c);
        }

        if (target->m_characters.size() > 1) {
            add_public_update<game_update_type::player_clear_characters>(target->id);
        }

        target->m_characters.resize(1);
        target->add_player_flags(player_flags::dead);
        target->m_hp = 0;

        if (!m_first_dead) m_first_dead = target;

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
    }

    void game::add_disabler(card *target_card, card_disabler_fun &&fun) {
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

        m_disablers.emplace(target_card, std::move(fun));
    }

    void game::remove_disablers(card *target_card) {
        const auto enable_if_old = [&](card *c) {
            bool a = false;
            bool b = false;
            for (const auto &[t, fun] : m_disablers) {
                if (t != target_card) a = a || fun(c);
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

        m_disablers.erase(target_card);
    }

    bool game::is_disabled(card *target_card) const {
        for (const auto &fun : m_disablers | std::views::values) {
            if (fun(target_card)) return true;
        }
        return false;
    }

    void game::handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args) {
        if (!m_requests.empty()) {
            auto &req = top_request();
            if (!req.target() || p == req.target()) {
                player *target_player = args.player_id ? get_player(args.player_id) : nullptr;
                card *target_card = args.card_id ? find_card(args.card_id) : nullptr;
                if (req.can_pick(args.pile, target_player, target_card)) {
                    req.on_pick(args.pile, target_player, target_card);
                }
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
}