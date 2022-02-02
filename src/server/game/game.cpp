#include "game.h"

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

    void game::send_character_update(const character &c, int player_id, int index) {
        player_character_update obj;
        obj.info = make_card_info(c);
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
        throw game_error("server.find_card: ID not found"_nonloc);
    }

    static auto make_id_vector(const auto &vec) {
        std::vector<int> ret;
        for (const card *obj : vec) {
            ret.push_back(obj->id);
        }
        return ret;
    };

    std::vector<game_update> game::get_game_state_updates(player *owner) {
        std::vector<game_update> ret;

        auto add_cards = [&](auto Fun, card_pile_type pile) {
            auto ids = m_cards
                | std::views::filter([&](const auto &pair) {
                    return Fun(pair.second.expansion);
                }) | std::views::keys;
            ret.emplace_back(enums::enum_constant<game_update_type::add_cards>{},
                std::vector(ids.begin(), ids.end()), pile);
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
            player_character_update obj;
            obj.max_hp = p.m_max_hp;
            obj.player_id = p.id;
            obj.index = 0;

            for (character *c : p.m_characters) {
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
            ret.emplace_back(enums::enum_constant<game_update_type::request_status>{},
                req.enum_index(),
                req.origin() ? req.origin()->id : 0,
                req.target() ? req.target()->id : 0,
                req.flags(),
                req.status_text());
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
        swap_testing<true>(m_deck);

        if (has_expansion(card_expansion_type::goldrush)) {
            for (const auto &c : all_cards.goldrush) {
                if (m_players.size() <= 2 && c.discard_if_two_players) continue;
                add_card(card_pile_type::shop_deck, c);
            }
            add_public_update<game_update_type::add_cards>(make_id_vector(m_shop_deck), card_pile_type::shop_deck);
            shuffle_cards_and_ids(m_shop_deck);
            swap_testing<true>(m_shop_deck);
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

        queue_event<event_type::on_game_start>();
        add_log("LOG_GAME_START");
        m_first_player = m_playing;
        m_first_player->start_of_turn();
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
}