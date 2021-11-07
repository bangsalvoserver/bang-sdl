#include "game.h"

#include "card.h"
#include "common/requests.h"
#include "common/net_enums.h"

#include <array>

namespace banggame {

    using namespace enums::flag_operators;

    static card_info make_card_info(const card &c) {
        auto make_card_info_effects = []<typename T>(std::vector<T> &out, const auto &vec) {
            for (const auto &value : vec) {
                T obj;
                obj.type = value.enum_index();
                obj.target = value.target();
                obj.args = value.args();
                out.push_back(std::move(obj));
            }
        };
        
        card_info info;
        info.expansion = c.expansion;
        info.id = c.id;
        info.name = c.name;
        info.image = c.image;
        info.playable_offturn = c.playable_offturn;
        info.modifier = c.modifier;
        info.optional_repeatable = c.optional_repeatable;
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

    void game::send_request_update() {
        const auto &req = top_request();
        add_public_update<game_update_type::request_handle>(
            req.enum_index(),
            req.origin_card() ? req.origin_card()->id : 0,
            req.origin() ? req.origin()->id : 0,
            req.target() ? req.target()->id : 0,
            req.target_card() ? req.target_card()->id : 0,
            req.flags());
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

    card *game::find_card(int card_id) {
        if (auto it = m_cards.find(card_id); it != m_cards.end()) {
            return &it->second;
        } else if (auto it = m_characters.find(card_id); it != m_characters.end()) {
            return &it->second;
        }
        throw game_error("server.find_card: id non trovato");
    }

    character *game::find_character(int card_id) {
        if (auto it = m_characters.find(card_id); it != m_characters.end()) {
            return &it->second;
        }
        throw game_error("server.find_character: id non trovato");
    }

    std::vector<game_update> game::get_game_state_updates() {
        std::vector<game_update> ret;

        // auto add_cards = [&ret](std::vector<card> vec) {
        //     auto pos = std::partition(vec.begin(), vec.end(), [](const card &c) { return c.expansion != card_expansion_type::goldrush; });
        //     if (pos != vec.begin()) {
        //         auto view = std::ranges::subrange(vec.begin(), pos) | std::views::transform(&card::id);
        //         ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{},
        //             std::vector(view.begin(), view.end()), card_pile_type::main_deck);
        //     }
        //     if (pos != vec.end()) {
        //         auto view = std::ranges::subrange(pos, vec.end()) | std::views::transform(&card::id);
        //         ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{}, std::vector(view.begin(), view.end()),
        //             pos->color == card_color_type::none ? card_pile_type::shop_hidden : card_pile_type::shop_deck);
        //     }
        // };

        // auto add_cards_to = [&](const std::vector<card> &vec, card_pile_type pile, player *owner = nullptr) {
        //     add_cards(vec);
        //     for (const auto &card : vec) {
        //         ret.emplace_back(enums::enum_constant<game_update_type::move_card>{},
        //             card.id, owner ? owner->id : 0, pile, show_card_flags::no_animation);
        //     }
        // };

        // auto add_cards_and_show = [&](const std::vector<card> &vec, card_pile_type pile, player *owner = nullptr) {
        //     add_cards_to(vec, pile, owner);

        //     for (const auto &c : vec) {
        //         show_card_update obj;
        //         obj.info = make_card_info(c);
        //         obj.suit = c.suit;
        //         obj.value = c.value;
        //         obj.color = c.color;
        //         obj.flags = show_card_flags::no_animation;

        //         ret.emplace_back(enums::enum_constant<game_update_type::show_card>{}, std::move(obj));
        //     }
        // };

        // add_cards(m_deck);
        // add_cards_to(m_discards, card_pile_type::discard_pile);

        // add_cards(m_shop_deck);
        // add_cards_to(m_shop_discards, card_pile_type::shop_discard);
        // add_cards_and_show(m_shop_selection, card_pile_type::shop_selection);
        // add_cards(m_shop_hidden);

        // add_cards(m_selection);

        // for (auto &p : m_players) {
        //     player_character_update obj;
        //     obj.max_hp = p.m_max_hp;
        //     obj.player_id = p.id;
        //     obj.index = 0;

        //     for (const auto &c : p.m_characters) {
        //         obj.type = c.type;
        //         obj.info = make_card_info(c);
        //         ret.emplace_back(enums::enum_constant<game_update_type::player_add_character>{}, obj);
        //         ++obj.index;
        //     }

        //     add_cards_to(p.m_hand, card_pile_type::player_hand, &p);
        //     add_cards_and_show(p.m_table, card_pile_type::player_table, &p);
        // }

        return ret;
    }

    void game::start_game(const game_options &options) {
        m_options = options;
        
        add_event<event_type::delayed_action>(0, [](std::function<void()> fun) { fun(); });
        
        std::random_device rd;
        rng.seed(rd());

        std::vector<character *> character_ptrs;
        for (const auto &c : all_cards.characters) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.expansions)) {
                auto it = m_characters.emplace(get_next_id(), c).first;
                character_ptrs.emplace_back(&it->second)->id = it->first;
            }
        }

        std::ranges::shuffle(character_ptrs, rng);
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

#ifdef TESTING_CHARACTER
        auto testing_char = std::ranges::find(character_ptrs, TESTING_CHARACTER, &character::image);
        std::swap(*character_it, *testing_char);
#endif
        std::ranges::shuffle(role_it, role_it + m_players.size(), rng);
        for (auto &p : m_players) {
            p.set_character_and_role(std::move(*character_it++), *role_it++);
        }

        for (; character_it != character_ptrs.end(); ++character_it) {
            if ((*character_it)->expansion == card_expansion_type::base) {
                m_base_characters.push_back(*character_it);
            }
        }

        for (const auto &c : all_cards.deck) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.expansions)) {
                auto it = m_cards.emplace(get_next_id(), c).first;
                m_deck.emplace_back(&it->second)->id = it->first;
            }
        }
        auto ids_view = m_deck | std::views::transform(&card::id);
        add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::main_deck);
        std::ranges::shuffle(m_deck, rng);

        if (has_expansion(card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                auto it = m_cards.emplace(get_next_id(), c).first;
                m_shop_deck.emplace_back(&it->second)->id = it->first;
            }
            ids_view = m_shop_deck | std::views::transform(&card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_deck);
            std::ranges::shuffle(m_shop_deck, rng);

            for (const auto &c : all_cards.goldrush_choices) {
                auto it = m_cards.emplace(get_next_id(), c).first;
                m_shop_hidden.emplace_back(&it->second)->id = it->first;
            }
            ids_view = m_shop_hidden | std::views::transform(&card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_hidden);
        }

        if (has_expansion(card_expansion_type::armedanddangerous)) {
            auto cube_ids = std::views::iota(1, 32);
            m_cubes.assign(cube_ids.begin(), cube_ids.end());
            add_public_update<game_update_type::add_cubes>(m_cubes);
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

    card *game::move_to(card *c, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (known) {
            send_card_update(*c, owner, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c->id, flags);
        }
        add_public_update<game_update_type::move_card>(c->id, owner ? owner->id : 0, pile, flags);
        return [this, pile, owner] () -> std::vector<card *>& {
            switch (pile) {
            case card_pile_type::player_hand:       return owner->m_hand;
            case card_pile_type::player_table:      return owner->m_table;
            case card_pile_type::main_deck:         return m_deck;
            case card_pile_type::discard_pile:      return m_discards;
            case card_pile_type::selection:         return m_selection;
            case card_pile_type::shop_selection:    return m_shop_selection;
            case card_pile_type::shop_discard:      return m_shop_discards;
            case card_pile_type::shop_hidden:       return m_shop_hidden;
            default: throw std::runtime_error("Pila non valida");
            }
        }().emplace_back(c);
    }

    card *game::draw_card_to(card_pile_type pile, player *owner, show_card_flags flags) {
        card *moved = move_to(m_deck.back(), pile, true, owner, flags);
        m_deck.pop_back();
        if (m_deck.empty()) {
            card *top_discards = m_discards.back();
            m_discards.resize(m_discards.size()-1);
            m_deck = std::move(m_discards);
            m_discards.clear();
            m_discards.emplace_back(std::move(top_discards));
            std::ranges::shuffle(m_deck, rng);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::main_deck);
        }
        return moved;
    }

    card *game::draw_shop_card() {
        card *moved = move_to(m_shop_deck.back(), card_pile_type::shop_selection);
        m_shop_deck.pop_back();
        if (m_shop_deck.empty()) {
            m_shop_deck = std::move(m_shop_discards);
            m_shop_discards.clear();
            std::ranges::shuffle(m_shop_deck, rng);
            add_public_update<game_update_type::deck_shuffled>(card_pile_type::shop_deck);
        }
        return moved;
    }

    card *game::draw_from_discards() {
        card *c = m_discards.back();
        m_discards.pop_back();
        return c;
    }

    card *game::draw_from_temp(card *c) {
        auto it = std::ranges::find(m_selection, c);
        if (it == m_selection.end()) throw game_error("server.draw_from_temp: ID non trovato");
        m_selection.erase(it);
        return c;
    }
    
    void game::draw_check_then(player *p, draw_check_function fun, bool force_one, bool invert_pop_req) {
        m_current_check.emplace(std::move(fun), p, force_one, invert_pop_req);
        do_draw_check();
    }

    void game::do_draw_check() {
        if (m_current_check->force_one || m_current_check->origin->m_num_checks == 1) {
            auto *c = draw_card_to(card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(c);
            instant_event<event_type::trigger_tumbleweed>(c->suit, c->value);
            if (!m_current_check->no_auto_resolve) {
                m_current_check->function(c->suit, c->value);
                m_current_check.reset();
            }
        } else {
            for (int i=0; i<m_current_check->origin->m_num_checks; ++i) {
                draw_card_to(card_pile_type::selection);
            }
            add_request<request_type::check>(0, m_current_check->origin, m_current_check->origin)
                .invert_pop_req = m_current_check->invert_pop_req;
        }
    }

    void game::resolve_check(card *c) {
        draw_from_temp(c);
        for (card *c : m_selection) {
            move_to(c, card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(c);
        }
        m_selection.clear();
        move_to(c, card_pile_type::discard_pile);
        queue_event<event_type::on_draw_check>(c);
        instant_event<event_type::trigger_tumbleweed>(c->suit, c->value);
        if (!m_current_check->no_auto_resolve) {
            m_current_check->function(c->suit, c->value);
            m_current_check.reset();
        }
    }

    void game::pop_request_noupdate() {
        enums::visit([](auto &value) {
            if constexpr (requires { value.cleanup(); }) {
                value.cleanup();
            }
        }, m_requests.front());
        m_requests.pop_front();
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
                if (target->m_role == player_role::outlaw && killer->m_role == player_role::renegade) {
                    return player_role::deputy;
                } else if (target->m_role == player_role::renegade && killer->m_role == player_role::deputy) {
                    return player_role::outlaw;
                } else if (target->m_role == player_role::deputy && killer->m_role == player_role::outlaw) {
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

    void game::player_death(player *target) {
        target->m_dead = true;
        target->m_hp = 0;

        instant_event<event_type::on_player_death>(m_playing, target);

        for (auto *c : target->m_characters) {
            c->on_unequip(target);
        }

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

    void game::disable_table_cards(bool disable_characters) {
        if (m_table_cards_disabled++ == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || p.id == m_playing->id) continue;
                for (auto *c : p.m_table) {
                    c->on_unequip(&p);
                }
                if (disable_characters && m_characters_disabled++ == 0) {
                    for (auto *c : p.m_characters) {
                        c->on_unequip(&p);
                    }
                }
            }
        }
    }

    void game::enable_table_cards(bool enable_characters) {
        if (--m_table_cards_disabled == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || p.id == m_playing->id) continue;
                for (auto *c : p.m_table) {
                    c->on_equip(&p);
                }
                if (enable_characters && --m_characters_disabled == 0) {
                    for (auto *c : p.m_characters) {
                        c->on_equip(&p);
                    }
                }
            }
        }
    }

    void game::handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args) {
        if (!m_requests.empty() && p == top_request().target()) {
            enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &req) {
                if constexpr (picking_request<E>) {
                    if (req.valid_pile(args.pile)) {
                        auto req_copy = req;
                        req_copy.on_pick(args.pile,
                            args.player_id ? get_player(args.player_id) : nullptr,
                            args.card_id ? find_card(args.card_id) : nullptr);
                    }
                }
            }, top_request());
        }
    }

    void game::handle_action(ACTION_TAG(play_card), player *p, const play_card_args &args) {
        if (bool(args.flags & play_card_flags::response)) {
            p->respond_card(args);
        } else if (m_requests.empty() && m_playing == p) {
            p->play_card(args);
        }
    }

    void game::handle_action(ACTION_TAG(draw_from_deck), player *p) {
        if (m_requests.empty() && m_playing == p && p->m_num_drawn_cards < p->m_num_cards_to_draw) {
            p->draw_from_deck();
        }
    }

    void game::handle_action(ACTION_TAG(pass_turn), player *p) {
        if (m_requests.empty() && m_playing == p && p->m_num_drawn_cards >= p->m_num_cards_to_draw) {
            p->pass_turn();
        }
    }

    void game::handle_action(ACTION_TAG(resolve), player *p) {
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