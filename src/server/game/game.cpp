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

        if (!owner || bool(flags & show_card_flags::show_everyone)) {
            add_public_update<game_update_type::show_card>(obj);
        } else {
            add_public_update<game_update_type::hide_card>(c.id, flags, owner->id);
            add_private_update<game_update_type::show_card>(owner, obj);
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

    std::vector<game_update> game::get_game_state_updates(player *owner) {
        std::vector<game_update> ret;

        auto is_goldrush = [](const auto &pair) {
            return pair.second.expansion == card_expansion_type::goldrush;
        };

        auto deck_card_ids = m_cards | std::views::filter(std::not_fn(is_goldrush)) | std::views::keys;
        ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{},
            std::vector(deck_card_ids.begin(), deck_card_ids.end()), card_pile_type::main_deck);

        auto shop_card_ids = m_cards | std::views::filter(is_goldrush) | std::views::keys;
        if (!std::ranges::empty(shop_card_ids)) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{},
                std::vector(shop_card_ids.begin(), shop_card_ids.end()), card_pile_type::shop_deck);
        }

        auto move_cards = [&](std::vector<card *> &vec, bool known = true) {
            for (card *c : vec) {
                ret.emplace_back(enums::enum_constant<game_update_type::move_card>{},
                    c->id, c->owner ? c->owner->id : 0, c->pile, show_card_flags::no_animation);

                if (known || c->owner == owner) {
                    show_card_update obj;
                    obj.info = make_card_info(*c);
                    obj.suit = c->suit;
                    obj.value = c->value;
                    obj.color = c->color;
                    obj.flags = show_card_flags::no_animation;

                    ret.emplace_back(enums::enum_constant<game_update_type::show_card>{}, std::move(obj));

                    for (int id : c->cubes) {
                        ret.emplace_back(enums::enum_constant<game_update_type::move_cube>{}, id, c->id);
                    }

                    if (c->inactive) {
                        ret.emplace_back(enums::enum_constant<game_update_type::tap_card>{}, c->id, true, true);
                    }
                }
            }
        };

        move_cards(m_discards);
        move_cards(m_selection, false);
        move_cards(m_shop_discards);
        move_cards(m_shop_selection);
        move_cards(m_shop_hidden);
        
        if (!m_cubes.empty()) {
            ret.emplace_back(enums::enum_constant<game_update_type::add_cubes>{}, m_cubes);
        }

        for (auto &p : m_players) {
            player_character_update obj;
            obj.max_hp = p.m_max_hp;
            obj.player_id = p.id;
            obj.index = 0;

            for (character *c : p.m_characters) {
                obj.type = c->type;
                obj.info = make_card_info(*c);
                ret.emplace_back(enums::enum_constant<game_update_type::player_add_character>{}, obj);
                ++obj.index;
            }

            for (int id : p.m_characters.front()->cubes) {
                ret.emplace_back(enums::enum_constant<game_update_type::move_cube>{}, id, p.m_characters.front()->id);
            }

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
            ret.emplace_back(enums::enum_constant<game_update_type::request_handle>{},
                req.enum_index(),
                req.origin_card() ? req.origin_card()->id : 0,
                req.origin() ? req.origin()->id : 0,
                req.target() ? req.target()->id : 0,
                req.target_card() ? req.target_card()->id : 0,
                req.flags());
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

    template<bool ToEnd, std::derived_from<card> T>
    static void swap_testing(std::vector<T *> &vec) {
        auto pos = [&]{
            if constexpr (ToEnd) {
                return vec.rbegin();
            } else {
                return vec.begin();
            }
        }();
        for (auto &ptr : vec | std::views::filter(&T::testing)) {
            std::swap(ptr, *pos++);
        }
    }

    void game::start_game(const game_options &options) {
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
        swap_testing<false>(character_ptrs);

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

        for (const auto &c : all_cards.deck) {
            if (m_players.size() <= 2 && c.discard_if_two_players) continue;
            if (bool(c.expansion & options.expansions)) {
                add_card(card_pile_type::main_deck, c);
            }
        }
        auto ids_view = m_deck | std::views::transform(&card::id);
        add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::main_deck);
        shuffle_cards_and_ids(m_deck);
        swap_testing<true>(m_deck);

        if (has_expansion(card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                add_card(card_pile_type::shop_deck, c);
            }
            ids_view = m_shop_deck | std::views::transform(&card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_deck);
            shuffle_cards_and_ids(m_shop_deck);
            swap_testing<true>(m_shop_deck);

            for (const auto &c : all_cards.goldrush_choices) {
                add_card(card_pile_type::shop_hidden, c);
            }
            ids_view = m_shop_hidden | std::views::transform(&card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::shop_hidden);
        }

        if (has_expansion(card_expansion_type::armedanddangerous)) {
            auto cube_ids = std::views::iota(1, 32);
            m_cubes.assign(cube_ids.begin(), cube_ids.end());
            add_public_update<game_update_type::add_cubes>(m_cubes);
        }

        std::vector<card *> last_scenario_cards;

        if (has_expansion(card_expansion_type::highnoon)) {
            for (const auto &c : all_cards.highnoon) {
                add_card(card_pile_type::scenario_deck, c);
            }
            last_scenario_cards.push_back(m_scenario_deck.back());
            m_scenario_deck.pop_back();
        }
        
        if (has_expansion(card_expansion_type::fistfulofcards)) {
            for (const auto &c : all_cards.fistfulofcards) {
                add_card(card_pile_type::scenario_deck, c);
            }
            last_scenario_cards.push_back(m_scenario_deck.back());
            m_scenario_deck.pop_back();
        }

        if (!m_scenario_deck.empty()) {
            shuffle_cards_and_ids(m_scenario_deck);
            m_scenario_deck.resize(12);
            m_scenario_deck.push_back(last_scenario_cards[std::uniform_int_distribution<>(0, last_scenario_cards.size() - 1)(rng)]);
            std::swap(m_scenario_deck.back(), m_scenario_deck.front());

            ids_view = m_scenario_deck | std::views::transform(&card::id);
            add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()), card_pile_type::scenario_deck);

            send_card_update(*m_scenario_deck.back(), nullptr, show_card_flags::no_animation);
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
        m_first_player = m_playing;
        m_first_player->start_of_turn(true);
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
        case card_pile_type::shop_hidden:       return m_shop_hidden;
        case card_pile_type::scenario_deck:     return m_scenario_deck;
        case card_pile_type::scenario_card:     return m_scenario_cards;
        default: throw std::runtime_error("Pila non valida");
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
        }
        return drawn_card;
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
        move_to(m_scenario_deck.back(), card_pile_type::scenario_card);
    }
    
    void game::draw_check_then(player *p, draw_check_function fun) {
        m_current_check.emplace(std::move(fun), p);
        do_draw_check();
    }

    void game::do_draw_check() {
        if (m_current_check->origin->m_num_checks == 1) {
            auto *c = draw_card_to(card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(c);
            card_suit_type suit = c->suit;
            card_value_type value = c->value;
            instant_event<event_type::apply_check_modifier>(suit, value);
            instant_event<event_type::trigger_tumbleweed>(suit, value);
            if (!m_current_check->no_auto_resolve) {
                m_current_check->function(suit, value);
                m_current_check.reset();
            }
        } else {
            for (int i=0; i<m_current_check->origin->m_num_checks; ++i) {
                draw_card_to(card_pile_type::selection);
            }
            add_request<request_type::check>(0, m_current_check->origin, m_current_check->origin);
        }
    }

    void game::resolve_check(card *c) {
        while (!m_selection.empty()) {
            card *drawn_card = m_selection.front();
            move_to(drawn_card, card_pile_type::discard_pile);
            queue_event<event_type::on_draw_check>(drawn_card);
        }
        card_suit_type suit = c->suit;
        card_value_type value = c->value;
        instant_event<event_type::apply_check_modifier>(suit, value);
        instant_event<event_type::trigger_tumbleweed>(suit, value);
        if (!m_current_check->no_auto_resolve) {
            m_current_check->function(suit, value);
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
        target->add_gold(-target->m_gold);

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

    void game::disable_table_cards() {
        if (m_table_cards_disabled++ == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || &p == m_playing) continue;
                for (auto *c : p.m_table) {
                    c->on_unequip(&p);
                }
            }
        }
    }

    void game::enable_table_cards() {
        if (--m_table_cards_disabled == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || &p == m_playing) continue;
                for (auto *c : p.m_table) {
                    c->on_equip(&p);
                }
            }
        }
    }

    bool game::table_cards_disabled(player *p) const {
        return m_table_cards_disabled > 0 && p != m_playing;
    }
    
    void game::disable_characters() {
        if (m_characters_disabled++ == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || &p == m_playing) continue;
                for (auto *c : p.m_characters) {
                    c->on_unequip(&p);
                }
            }
        }
    }
    
    void game::enable_characters() {
        if (--m_characters_disabled == 0) {
            for (auto &p : m_players) {
                if (!p.alive() || &p == m_playing) continue;
                for (auto *c : p.m_characters) {
                    c->on_equip(&p);
                }
            }
        }
    }

    bool game::characters_disabled(player *p) const {
        return m_characters_disabled > 0 && p != m_playing;
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