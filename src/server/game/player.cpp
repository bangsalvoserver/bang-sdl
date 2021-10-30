#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/requests.h"
#include "common/net_enums.h"

namespace banggame {
    using namespace enums::flag_operators;

    player::player(game *game)
        : m_game(game)
        , id(game->get_next_id()) {}
    
    void player::discard_weapon(int card_id) {
        auto it = std::ranges::find_if(m_table, [card_id](const deck_card &c) {
            return !c.equips.empty() && c.equips.front().is(equip_type::weapon) && c.id != card_id;
        });
        if (it != m_table.end()) {
            drop_all_cubes(*it);
            m_game->move_to(std::move(*it), card_pile_type::discard_pile).on_unequip(this);
            m_table.erase(it);
        }
    }

    deck_card &player::equip_card(deck_card &&target) {
        target.on_equip(this);
        auto &moved = m_table.emplace_back(std::move(target));
        m_game->send_card_update(moved);
        m_game->add_public_update<game_update_type::move_card>(moved.id, id, card_pile_type::player_table);
        return moved;
    }

    bool player::has_card_equipped(const std::string &name) const {
        return std::ranges::find(m_table, name, &deck_card::name) != m_table.end();
    }

    deck_card &player::random_hand_card() {
        return m_hand[m_game->rng() % m_hand.size()];
    }

    deck_card &player::find_card(int card_id) {
        if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            return *it;
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            return *it;
        } else {
            throw game_error("server.find_card: ID non trovato");
        }
    }

    character &player::find_character(int card_id) {
        if (auto it = std::ranges::find(m_characters, card_id, &character::id); it != m_characters.end()) {
            return *it;
        } else {
            throw game_error("server.find_character: ID non trovato");
        }
    }

    deck_card &player::discard_card(int card_id) {
        if (auto it = std::ranges::find(m_table, card_id, &card::id); it != m_table.end()) {
            if (it->inactive) {
                it->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(it->id, false);
            }
            drop_all_cubes(*it);
            auto &moved = m_game->move_to(std::move(*it), it->color == card_color_type::black
                ? card_pile_type::shop_discard
                : card_pile_type::discard_pile);
            m_table.erase(it);
            m_game->queue_event<event_type::post_discard_card>(this, card_id);
            moved.on_unequip(this);
            return moved;
        } else if (auto it = std::ranges::find(m_hand, card_id, &card::id); it != m_hand.end()) {
            auto &moved = m_game->move_to(std::move(*it), card_pile_type::discard_pile);
            m_hand.erase(it);
            m_game->queue_event<event_type::post_discard_card>(this, card_id);
            return moved;
        } else {
            throw game_error("server.discard_card: ID non trovato");
        }
    }

    deck_card &player::steal_card(player *target, int card_id) {
        if (auto it = std::ranges::find(target->m_table, card_id, &card::id); it != target->m_table.end()) {
            if (it->inactive) {
                it->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(it->id, false);
            }
            drop_all_cubes(*it);
            auto &moved = add_to_hand(std::move(*it));
            target->m_table.erase(it);
            m_game->queue_event<event_type::post_discard_card>(target, card_id);
            moved.on_unequip(target);
            return moved;
        } else if (auto it = std::ranges::find(target->m_hand, card_id, &card::id); it != target->m_hand.end()) {
            auto &moved = add_to_hand(std::move(*it));
            target->m_hand.erase(it);
            m_game->queue_event<event_type::post_discard_card>(target, card_id);
            return moved;
        } else {
            throw game_error("server.steal_card: ID non trovato");
        }
    }

    void player::damage(int origin_card_id, player *source, int value, bool is_bang) {
        if (!m_ghost) {
            if (m_game->has_expansion(card_expansion_type::valleyofshadows)) {
                auto &obj = m_game->queue_request<request_type::damaging>(origin_card_id, source, this);
                obj.damage = value;
                obj.is_bang = is_bang;
            } else {
                do_damage(origin_card_id, source, value, is_bang);
            }
        }
    }

    void player::do_damage(int origin_card_id, player *origin, int value, bool is_bang) {
        m_hp -= value;
        m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        if (m_hp <= 0) {
            m_game->add_request<request_type::death>(origin_card_id, origin, this);
        }
        if (m_game->has_expansion(card_expansion_type::goldrush)) {
            if (origin && origin->m_game->m_playing == origin && origin != this) {
                origin->add_gold(value);
            }
        }
        m_game->queue_event<event_type::on_hit>(origin, this, value, is_bang);
    }

    void player::heal(int value) {
        if (!m_ghost) {
            m_hp = std::min(m_hp + value, m_max_hp);
            m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        }
    }

    void player::add_gold(int amount) {
        m_gold += amount;
        m_game->add_public_update<game_update_type::player_gold>(id, m_gold);
    }

    bool player::can_receive_cubes() const {
        if (m_game->m_cubes.empty()) return false;
        if (m_characters.front().cubes.size() < 4) return true;
        return std::ranges::any_of(m_table, [](const deck_card &card) {
            return card.color == card_color_type::orange && card.cubes.size() < 4;
        });
    }
    
    void player::add_cubes(card &target, int ncubes) {
        for (;ncubes!=0 && !m_game->m_cubes.empty() && target.cubes.size() < 4; --ncubes) {
            int cube = m_game->m_cubes.back();
            m_game->m_cubes.pop_back();

            target.cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, target.id);
        }
    }

    void player::pay_cubes(card &target, int ncubes) {
        for (;ncubes!=0 && !target.cubes.empty(); --ncubes) {
            int cube = target.cubes.back();
            target.cubes.pop_back();

            m_game->m_cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, 0);
        }
        m_game->instant_event<event_type::on_pay_cube>(target.id, target.cubes.size());
        if (target.cubes.empty() && target.id != m_characters.front().id) {
            m_game->queue_event<event_type::delayed_action>([this, card_id = target.id]{
                discard_card(card_id);
            });
        }
    }

    void player::move_cubes(card &origin, card &target, int ncubes) {
        for(;ncubes!=0 && !origin.cubes.empty(); --ncubes) {
            int cube = origin.cubes.back();
            origin.cubes.pop_back();
            
            if (target.cubes.size() < 4) {
                target.cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, target.id);
            } else {
                m_game->m_cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, 0);
            }
        }
        m_game->instant_event<event_type::on_pay_cube>(origin.id, origin.cubes.size());
        if (origin.cubes.empty() && origin.id != m_characters.front().id) {
            m_game->queue_event<event_type::delayed_action>([this, card_id = origin.id]{
                discard_card(card_id);
            });
        }
    }

    void player::drop_all_cubes(card &target) {
        for (int id : target.cubes) {
            m_game->m_cubes.push_back(id);
            m_game->add_public_update<game_update_type::move_cube>(id, 0);
        }
        target.cubes.clear();
    }


    deck_card &player::add_to_hand(deck_card &&target) {
        return m_game->move_to(std::move(target), card_pile_type::player_hand, true, this);
    }

    static bool player_in_range(player *origin, player *target, int distance) {
        return origin->m_game->calc_distance(origin, target) <= distance;
    }

    void player::verify_and_play_modifier(const card &c, int modifier_id) {
        if (!modifier_id) return;
        auto &mod_card = find_card(modifier_id);
        switch(mod_card.modifier) {
        case card_modifier_type::anycard:
            break;
        case card_modifier_type::bangcard:
            if (std::ranges::find(c.effects, effect_type::bangcard, &effect_holder::enum_index) != c.effects.end())
                break;
            [[fallthrough]];
        default:
            throw game_error("Azione non valida");
        }
        for (const auto &e : mod_card.effects) {
            if (!e.can_play(modifier_id, this)) {
                throw game_error("Azione non valida");
            }
        }
        do_play_card(modifier_id, false, std::vector{mod_card.effects.size(),
            play_card_target{enums::enum_constant<play_card_target_type::target_none>{}}});
    }

    void player::verify_equip_target(const card &c, const std::vector<play_card_target> &targets) {
        if (targets.size() != 1) throw game_error("Azione non valida");
        if (targets.front().enum_index() != play_card_target_type::target_player) throw game_error("Azione non valida");
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (c.equips.empty()) return;
        if (tgts.size() != 1) throw game_error("Azione non valida");
        player *target = m_game->get_player(tgts.front().player_id);
        if (!std::ranges::all_of(enums::enum_values_v<target_type>
            | std::views::filter([type = c.equips.front().target()](target_type value) {
                return bool(type & value);
            }), [&](target_type value) {
                switch (value) {
                case target_type::player: return target->alive();
                case target_type::dead: return !target->alive();
                case target_type::self: return target->id == id;
                case target_type::notself: return target->id != id;
                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                case target_type::maxdistance: return player_in_range(this, target, c.equips.front().args() + m_range_mod);
                default: return false;
                }
            })) throw game_error("Azione non valida");
    }

    static bool is_new_target(const std::multimap<int, int> &targets, int card_id, int player_id) {
        auto [lower, upper] = targets.equal_range(card_id);
        return std::ranges::find(lower, upper, player_id, [](const auto &pair) { return pair.second; }) == upper;
    }

    void player::verify_card_targets(const card &c, bool is_response, const std::vector<play_card_target> &targets) {
        auto &effects = is_response ? c.responses : c.effects;
        if (c.cost > m_gold) throw game_error("Non hai abbastanza pepite");
        if (c.max_usages != 0 && c.usages >= c.max_usages) throw game_error("Azione non valida");

        if (effects.size() != targets.size()) throw game_error("Target non validi");
        if (!std::ranges::all_of(effects, [&, it = targets.begin()] (const effect_holder &e) mutable {
            return enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    if (!e.can_play(c.id, this)) return false;
                    return e.target() == enums::flags_none<target_type>;
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    if (!bool(e.target() & (target_type::player | target_type::dead))) return false;
                    if (bool(e.target() & target_type::everyone)) {
                        std::vector<target_player_id> ids;
                        if (bool(e.target() & target_type::notself)) {
                            for (auto *p = this;;) {
                                p = m_game->get_next_player(p);
                                if (p == this) break;
                                if (!e.can_play(c.id, this, p)) return false;
                                ids.emplace_back(p->id);
                            }
                        } else {
                            for (auto *p = this;;) {
                                ids.emplace_back(p->id);
                                if (!e.can_play(c.id, this, p)) return false;
                                p = m_game->get_next_player(p);
                                if (p == this) break;
                            }
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    } else if (args.size() != 1) {
                        return false;
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        if (!e.can_play(c.id, this, target)) return false;
                        return std::ranges::all_of(enums::enum_values_v<target_type>
                            | std::views::filter([type = e.target()](target_type value) {
                                return bool(type & value);
                            }), [&](target_type value) {
                                switch (value) {
                                case target_type::player: return target->alive();
                                case target_type::dead: return !target->alive();
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args() + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, c.id, target->id);
                                case target_type::fanning_target: {
                                    player *prev_target = m_game->get_player((it - 2)->get<play_card_target_type::target_player>().front().player_id);
                                    return player_in_range(prev_target, target, 1) && target != prev_target;
                                }
                                default: return false;
                                }
                            });
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    if (!bool(e.target() & target_type::card)) return false;
                    if (bool(e.target() & target_type::everyone)) {
                        if (!std::ranges::all_of(m_game->m_players | std::views::filter(&player::alive), [&](int player_id) {
                            bool found = std::ranges::find(args, player_id, &target_card_id::player_id) != args.end();
                            if (bool(e.target() & target_type::notself)) {
                                return (player_id == id) != found;
                            } else {
                                return found;
                            }
                        }, &player::id)) return false;
                        return std::ranges::all_of(args, [&](const target_card_id &arg) {
                            if (arg.player_id == id) return false;
                            auto *target = m_game->get_player(arg.player_id);
                            if (!e.can_play(c.id, this, target, arg.card_id)) return false;
                            if (arg.from_hand) return true;
                            return std::ranges::find(target->m_table, arg.card_id, &deck_card::id) != target->m_table.end();
                        });
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        if (!e.can_play(c.id, this, target, args.front().card_id)) return false;
                        return std::ranges::all_of(enums::enum_values_v<target_type>
                            | std::views::filter([type = e.target()](target_type value) {
                                return bool(type & value);
                            }), [&](target_type value) {
                                switch (value) {
                                case target_type::card: return target->alive() && target->find_card(args.front().card_id).color != card_color_type::black;
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args() + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, c.id, target->id);
                                case target_type::fanning_target: {
                                    player *prev_target = m_game->get_player((it - 2)->get<play_card_target_type::target_player>().front().player_id);
                                    return player_in_range(prev_target, target, 1);
                                }
                                case target_type::table: return !args.front().from_hand;
                                case target_type::hand: return args.front().from_hand;
                                case target_type::blue: return target->find_card(args.front().card_id).color == card_color_type::blue;
                                case target_type::clubs: return target->find_card(args.front().card_id).suit == card_suit_type::clubs;
                                case target_type::bang: {
                                    auto &c = target->find_card(args.front().card_id);
                                    return !c.effects.empty() && c.effects.front().is(effect_type::bangcard);
                                }
                                case target_type::missed: {
                                    auto &c = target->find_card(args.front().card_id);
                                    return !c.responses.empty() && c.responses.front().is(effect_type::missedcard);
                                }
                                case target_type::bangormissed: {
                                    auto &c = target->find_card(args.front().card_id);
                                    if (!c.effects.empty()) return c.effects.front().is(effect_type::bangcard);
                                    else if (!c.responses.empty()) return c.responses.front().is(effect_type::missedcard);
                                }
                                case target_type::cube_slot:
                                    return args.front().card_id == target->m_characters.front().id
                                        || target->find_card(args.front().card_id).color == card_color_type::orange;
                                default: return false;
                                }
                            });
                    }
                }
            }, *it++);
        })) throw game_error("Azione non valida");
    }

    void player::do_play_card(int card_id, bool is_response, const std::vector<play_card_target> &targets) {
        card *card_ptr = nullptr;
        bool is_character = false;
        bool is_virtual = false;
        if (auto it = std::ranges::find(m_characters, card_id, &character::id); it != m_characters.end()) {
            card_ptr = &*it;
            is_character = true;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            auto &moved = m_game->move_to(std::move(*it), card_pile_type::discard_pile);
            card_ptr = &moved;
            m_last_played_card = card_id;
            m_hand.erase(it);
            m_game->queue_event<event_type::on_play_hand_card>(this, card_id);
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            if (m_game->table_cards_disabled(id)) throw game_error("Le carte in gioco sono disabilitate");
            m_last_played_card = card_id;
            if (it->color == card_color_type::green) {
                card_ptr = &m_game->move_to(std::move(*it), card_pile_type::discard_pile);
                m_table.erase(it);
            } else {
                card_ptr = &*it;
            }
        } else if (auto it = std::ranges::find(m_game->m_shop_selection, card_id, &deck_card::id); it != m_game->m_shop_selection.end()) {
            if (it->color == card_color_type::brown) {
                m_last_played_card = card_id;
                card_ptr = &m_game->move_to(std::move(*it), card_pile_type::shop_discard);
                m_game->m_shop_selection.erase(it);
            } else {
                card_ptr = &*it;
            }
        } else if (m_virtual && card_id == m_virtual->first) {
            card_ptr = &m_virtual->second;
            is_virtual = true;
            m_last_played_card = 0;
            if (auto it = std::ranges::find(m_hand, m_virtual->second.id, &card::id); it != m_hand.end()) {
                m_game->move_to(std::move(*it), card_pile_type::discard_pile);
                m_hand.erase(it);
                m_game->queue_event<event_type::on_play_hand_card>(this, m_virtual->second.id);
            }
        }

        if (!card_ptr) {
            throw game_error("server.do_play_card: ID non trovato");
        }

        if (card_ptr->max_usages != 0) {
            ++card_ptr->usages;
        }

        auto check_immunity = [&](player *target) {
            if (m_virtual) {
                return target->immune_to(m_virtual->second);
            }
            if (is_character) return false;
            return target->immune_to(*static_cast<deck_card*>(card_ptr));
        };

        size_t initial_mods_size = m_bang_mods.size();
        
        if (card_ptr->cost > 0) {
            add_gold(-card_ptr->cost);
        }
        
        auto effect_it = targets.begin();
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        for (auto &e : effects) {
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    e.on_play(card_id, this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        m_current_card_targets.emplace(card_id, target.player_id);
                        e.on_play(card_id, this, p);
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        m_current_card_targets.emplace(card_id, target.player_id);
                        if (target.from_hand && p != this) {
                            e.on_play(card_id, this, p, p->random_hand_card().id);
                        } else {
                            e.on_play(card_id, this, p, target.card_id);
                        }
                    }
                }
            }, *effect_it++);
        }

        m_game->queue_event<event_type::on_effect_end>(this);
        if (is_virtual) {
            m_virtual.reset();
        }

        if (m_bang_mods.size() == initial_mods_size) {
            m_bang_mods.clear();
        }
    }

    void player::play_card(const play_card_args &args) {
        if (bool(args.flags & play_card_flags::sell_beer)) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (!m_game->has_expansion(card_expansion_type::goldrush)
                || args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw game_error("Azione non valida");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw game_error("Azione non valida");
            const auto &c = find_card(l.front().card_id);
            if (c.effects.empty() || !c.effects.front().is(effect_type::beer)) throw game_error("Azione non valida");
            m_game->add_log(this, nullptr, std::string("venduto ") + c.name);
            discard_card(c.id);
            add_gold(1);
            m_game->queue_event<event_type::on_play_beer>(this);
            m_game->queue_event<event_type::on_effect_end>(this);
        } else if (bool(args.flags & play_card_flags::discard_black)) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw game_error("Azione non valida");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw game_error("Azione non valida");
            auto *p = m_game->get_player(l.front().player_id);
            auto &c = p->find_card(l.front().card_id);
            if (c.color != card_color_type::black) throw game_error("Azione non valida");
            if (m_gold < c.buy_cost + 1) throw game_error("Non hai abbastanza pepite");
            m_game->add_log(this, p, std::string("scartato ") + c.name);
            add_gold(-c.buy_cost - 1);
            p->discard_card(c.id);
        } else if (auto card_it = std::ranges::find(m_characters, args.card_id, &character::id); card_it != m_characters.end()) {
            switch (card_it->type) {
            case character_type::active:
                if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
                verify_card_targets(*card_it, false, args.targets);
                verify_and_play_modifier(*card_it, args.modifier_id);
                m_game->add_log(this, nullptr, std::string("giocato effetto di ") + card_it->name);
                do_play_card(args.card_id, false, args.targets);
                break;
            case character_type::drawing:
            case character_type::drawing_forced:
                if (m_num_drawn_cards >= m_num_cards_to_draw) throw game_error("Non devi pescare adesso");
                verify_card_targets(*card_it, false, args.targets);
                m_game->add_private_update<game_update_type::status_clear>(this);
                m_game->add_log(this, nullptr, std::string("pescato con l'effetto di ") + card_it->name);
                do_play_card(args.card_id, false, args.targets);
                break;
            default:
                break;
            }
        } else if (m_virtual && args.card_id == m_virtual->first) {
            verify_card_targets(m_virtual->second, false, args.targets);
            verify_and_play_modifier(*card_it, args.modifier_id);
            do_play_card(args.card_id, false, args.targets);
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            switch (card_it->color) {
            case card_color_type::brown:
                verify_card_targets(*card_it, false, args.targets);
                verify_and_play_modifier(*card_it, args.modifier_id);
                m_game->add_log(this, nullptr, std::string("giocato ") + card_it->name);
                do_play_card(args.card_id, false, args.targets);
                break;
            case card_color_type::blue: {
                verify_equip_target(*card_it, args.targets);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                if (target->has_card_equipped(card_it->name)) throw game_error("Carta duplicata");
                m_game->add_log(this, target, std::string("equipaggiato ") + card_it->name);
                deck_card removed = std::move(*card_it);
                m_hand.erase(card_it);
                target->equip_card(std::move(removed));
                if (m_game->has_expansion(card_expansion_type::armedanddangerous) && can_receive_cubes()) {
                    m_game->queue_request<request_type::add_cube>(0, nullptr, this);
                }
                m_game->queue_event<event_type::on_equip>(this, target, args.card_id);
                m_game->queue_event<event_type::on_effect_end>(this);
                break;
            }
            case card_color_type::green: {
                if (has_card_equipped(card_it->name)) throw game_error("Carta duplicata");
                m_game->add_log(this, nullptr, std::string("equipaggiato ") + card_it->name);
                card_it->inactive = true;
                deck_card removed = std::move(*card_it);
                m_hand.erase(card_it);
                equip_card(std::move(removed));
                m_game->queue_event<event_type::on_equip>(this, this, args.card_id);
                m_game->queue_event<event_type::on_effect_end>(this);
                m_game->add_public_update<game_update_type::tap_card>(removed.id, true);
                break;
            }
            case card_color_type::orange: {
                verify_equip_target(*card_it, args.targets);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                if (target->has_card_equipped(card_it->name)) throw game_error("Carta duplicata");
                m_game->add_log(this, target, std::string("equipaggiato ") + card_it->name);
                deck_card removed = std::move(*card_it);
                m_hand.erase(card_it);
                add_cubes(target->equip_card(std::move(removed)), 3);
                m_game->queue_event<event_type::on_equip>(this, this, args.card_id);
                m_game->queue_event<event_type::on_effect_end>(this);
            }
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (card_it->inactive) throw game_error("Carta non attiva in questo turno");
            verify_card_targets(*card_it, false, args.targets);
            verify_and_play_modifier(*card_it, args.modifier_id);
            m_game->add_log(this, nullptr, std::string("giocato ") + card_it->name + " da terra");
            do_play_card(args.card_id, false, args.targets);
        } else if (auto card_it = std::ranges::find(m_game->m_shop_selection, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            int discount = 0;
            if (auto modifier_it = std::ranges::find(m_characters, args.modifier_id, &character::id); modifier_it != m_characters.end()) {
                if (modifier_it->modifier == card_modifier_type::discount
                    && modifier_it->usages < modifier_it->max_usages) {
                    discount = 1;
                } else {
                    throw game_error("Azione non valida");
                }
            }
            if (m_gold >= card_it->buy_cost - discount) {
                switch (card_it->color) {
                case card_color_type::brown:
                    verify_card_targets(*card_it, false, args.targets);
                    if (args.modifier_id) do_play_card(args.modifier_id, false, {});
                    add_gold(discount - card_it->buy_cost);
                    m_game->add_log(this, nullptr, std::string("comprato e giocato ") + card_it->name);
                    do_play_card(args.card_id, false, args.targets);
                    m_game->queue_event<event_type::delayed_action>([this]{
                        m_game->draw_shop_card();
                    });
                break;
                case card_color_type::black:
                    verify_equip_target(*card_it, args.targets);
                    if (args.modifier_id) do_play_card(args.modifier_id, false, {});
                    auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (target->has_card_equipped(card_it->name)) throw game_error("Carta duplicata");
                    m_game->add_log(this, target, std::string("comprato e equipaggiato ") + card_it->name);
                    add_gold(discount - card_it->buy_cost);
                    deck_card removed = std::move(*card_it);
                    m_game->m_shop_selection.erase(card_it);
                    target->equip_card(std::move(removed));
                    m_game->draw_shop_card();
                    m_game->queue_event<event_type::on_effect_end>(this);
                    break;
                }
            } else {
                throw game_error("Non hai abbastanza pepite");
            }
        } else {
            throw game_error("server.play_card: ID non trovato");
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        auto can_respond = [&](const card &c) {
            return std::ranges::any_of(c.responses, [&](const effect_holder &e) {
                return e.can_respond(this);
            });
        };

        if (auto card_it = std::ranges::find(m_characters, args.card_id, &character::id); card_it != m_characters.end()) {
            if (can_respond(*card_it)) {
                verify_card_targets(*card_it, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con l'effetto di ") + card_it->name);
                do_play_card(args.card_id, true, args.targets);
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            if (card_it->color == card_color_type::brown && can_respond(*card_it)) {
                verify_card_targets(*card_it, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con ") + card_it->name);
                do_play_card(args.card_id, true, args.targets);
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (m_game->table_cards_disabled(id)) throw game_error("Le carte in gioco sono disabilitate");
            if (card_it->inactive) throw game_error("Carta non attiva in questo turno");
            if (can_respond(*card_it)) {
                verify_card_targets(*card_it, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con ") + card_it->name + " da terra");
                do_play_card(args.card_id, true, args.targets);
            }
        } else if (auto card_it = std::ranges::find(m_game->m_shop_selection, args.card_id, &deck_card::id); card_it != m_table.end()) {
            // hack per bottiglia e complice
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            verify_card_targets(*card_it, true, args.targets);
            m_game->add_log(this, nullptr, std::string("scelto opzione ") + card_it->name);
            do_play_card(args.card_id, true, args.targets);
        } else {
            throw game_error("server.respond_card: ID non trovato");
        }
    }

    void player::draw_from_deck() {
        if (std::ranges::find(m_characters, character_type::drawing_forced, &character::type) == m_characters.end()) {
            for (; m_num_drawn_cards<m_num_cards_to_draw; ++m_num_drawn_cards) {
                m_game->draw_card_to(card_pile_type::player_hand, this);
            }
            m_game->add_log(this, nullptr, "pescato dal mazzo");
            m_game->queue_event<event_type::on_draw_from_deck>(this);
            m_game->add_private_update<game_update_type::status_clear>(this);
        }
    }

    void player::play_virtual_card(deck_card c) {
        virtual_card_update obj;
        obj.virtual_id = m_game->get_next_id();
        obj.card_id = c.id;
        obj.color = c.color;
        obj.suit = c.suit;
        obj.value = c.value;
        for (const auto &value : c.effects) {
            card_target_data ctd;
            ctd.type = value.enum_index();
            ctd.target = value.target();
            ctd.args = value.args();
            obj.targets.emplace_back(std::move(ctd));
        }
        m_virtual = std::make_pair(obj.virtual_id, std::move(c));

        m_game->add_private_update<game_update_type::virtual_card>(this, obj);
    }

    void player::start_of_turn() {
        m_game->m_next_turn = nullptr;
        m_game->m_playing = this;

        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        for (auto &c : m_characters) {
            c.usages = 0;
        }
        for (auto &c : m_table) {
            c.usages = 0;
        }

        m_current_card_targets.clear();
        
        m_game->add_public_update<game_update_type::switch_turn>(id);

        if (m_predraw_checks.empty()) {
            m_game->queue_event<event_type::on_turn_start>(this);
        } else {
            for (auto &[card_id, obj] : m_predraw_checks) {
                obj.resolved = false;
            }
            m_game->queue_request<request_type::predraw>(0, this, this);
        }
    }

    void player::next_predraw_check(int card_id) {
        m_game->queue_event<event_type::delayed_action>([this, card_id]{
            if (auto it = m_predraw_checks.find(card_id); it != m_predraw_checks.end()) {
                it->second.resolved = true;
            }

            if (std::ranges::all_of(m_predraw_checks | std::views::values, &predraw_check::resolved)) {
                m_game->queue_event<event_type::on_turn_start>(this);
            } else {
                m_game->queue_request<request_type::predraw>(0, this, this);
            }
        });
    }

    void player::pass_turn() {
        if (num_hand_cards() > max_cards_end_of_turn()) {
            m_game->queue_request<request_type::discard_pass>(0, this, this);
        } else {
            end_of_turn();
        }
    }

    void player::end_of_turn() {
        for (auto &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c.id, false);
            }
        }
        m_current_card_targets.clear();
        m_game->queue_event<event_type::on_turn_end>(this);
        m_game->queue_event<event_type::delayed_action>([this]{
            if (m_game->num_alive() > 0) {
                if (m_game->m_next_turn) {
                    m_game->m_next_turn->start_of_turn();
                } else {
                    m_game->get_next_player(this)->start_of_turn();
                }
            }
        });
    }

    void player::discard_all() {
        for (deck_card &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c.id, false);
            }
            drop_all_cubes(c);
            c.on_unequip(this);
            m_game->move_to(std::move(c), c.color == card_color_type::black
                ? card_pile_type::shop_discard
                : card_pile_type::discard_pile);
        }
        m_table.clear();
        for (deck_card &c : m_hand) {
            m_game->move_to(std::move(c), card_pile_type::discard_pile);
        }
        m_hand.clear();
    }

    void player::set_character_and_role(character &&c, player_role role) {
        m_role = role;

        m_max_hp = c.max_hp;
        if (role == player_role::sheriff) {
            ++m_max_hp;
        }
        m_hp = m_max_hp;

        auto &moved = m_characters.emplace_back(std::move(c));
        moved.on_equip(this);
        m_game->send_character_update(moved, id, 0);

        if (role == player_role::sheriff || m_game->m_players.size() <= 3) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role, true);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role, true);
        }
    }
}