#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/requests.h"
#include "common/net_enums.h"

#include <cassert>

namespace banggame {
    using namespace enums::flag_operators;

    player::player(game *game)
        : m_game(game)
        , id(game->get_next_id()) {}

    void player::equip_card(card *target) {
        auto is_weapon = [](const card *c) {
            return !c->equips.empty() && c->equips.front().is(equip_type::weapon);
        };

        if (is_weapon(target)) {
            if (auto it = std::ranges::find_if(m_table, is_weapon); it != m_table.end()) {
                discard_card(*it);
            }
        }

        if (!m_game->table_cards_disabled(this)) {
            target->on_equip(this);
        }
        m_game->move_to(target, card_pile_type::player_table, true, this, show_card_flags::show_everyone);
    }

    bool player::alive() const {
        return !m_dead || m_ghost
            || (m_game->m_playing == this && m_game->has_scenario(scenario_flags::ghosttown));
    }

    bool player::has_card_equipped(const std::string &name) const {
        return std::ranges::find(m_table, name, &card::name) != m_table.end();
    }

    card *player::random_hand_card() {
        return m_hand[std::uniform_int_distribution<int>(0, m_hand.size() - 1)(m_game->rng)];
    }

    std::vector<card *>::iterator player::move_card_to(card *target_card, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (target_card->owner != this) throw localized_game_error("ERROR_PLAYER_DOES_NOT_OWN_CARD");
        if (target_card->pile == card_pile_type::player_table) {
            if (target_card->inactive) {
                target_card->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(target_card->id, false);
            }
            drop_all_cubes(target_card);
            auto it = m_game->move_to(target_card, pile, known, owner, flags);
            m_game->queue_event<event_type::post_discard_card>(this, target_card);
            if (!m_game->table_cards_disabled(this)) {
                target_card->on_unequip(this);
            }
            return it;
        } else if (target_card->pile == card_pile_type::player_hand) {
            return m_game->move_to(target_card, pile, known, owner, flags);
        } else {
            throw localized_game_error("ERROR_CARD_NOT_FOUND");
        }
    }

    void player::discard_card(card *target) {
        move_card_to(target, target->color == card_color_type::black
            ? card_pile_type::shop_discard
            : card_pile_type::discard_pile, true);
    }

    void player::steal_card(player *target, card *target_card) {
        target->move_card_to(target_card, card_pile_type::player_hand, true, this);
    }

    void player::damage(card *origin_card, player *source, int value, bool is_bang) {
        if (!m_ghost && !(m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown))) {
            if (m_game->has_expansion(card_expansion_type::valleyofshadows)) {
                auto &obj = m_game->queue_request<request_type::damaging>(origin_card, source, this);
                obj.damage = value;
                obj.is_bang = is_bang;
            } else {
                do_damage(origin_card, source, value, is_bang);
            }
        }
    }

    void player::do_damage(card *origin_card, player *origin, int value, bool is_bang) {
        m_hp -= value;
        m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        m_game->add_log(value == 1 ? "LOG_TAKEN_DAMAGE" : "LOG_TAKEN_DAMAGE_PLURAL", origin_card, this, nullptr, nullptr, value);
        if (m_hp <= 0) {
            m_game->add_request<request_type::death>(origin_card, origin, this);
        }
        if (m_game->has_expansion(card_expansion_type::goldrush)) {
            if (origin && origin->m_game->m_playing == origin && origin != this) {
                origin->add_gold(value);
            }
        }
        m_game->queue_event<event_type::on_hit>(origin_card, origin, this, value, is_bang);
    }

    void player::heal(int value) {
        if (!m_ghost && !(m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown))) {
            m_hp = std::min(m_hp + value, m_max_hp);
            m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        }
    }

    void player::add_gold(int amount) {
        m_gold += amount;
        m_game->add_public_update<game_update_type::player_gold>(id, m_gold);
    }

    bool player::immune_to(card *c) {
        return m_calumets > 0 && get_card_suit(c) == card_suit_type::diamonds;
    }

    bool player::can_receive_cubes() const {
        if (m_game->m_cubes.empty()) return false;
        if (m_characters.front()->cubes.size() < 4) return true;
        return std::ranges::any_of(m_table, [](const card *card) {
            return card->color == card_color_type::orange && card->cubes.size() < 4;
        });
    }

    bool player::can_escape(player *origin, card *origin_card, effect_flags flags) const {
        // hack per determinare se e' necessario attivare request di fuga
        if (bool(flags & effect_flags::escapable)
            && m_game->has_expansion(card_expansion_type::valleyofshadows)) return true;
        auto it = std::ranges::find_if(m_characters, [](const auto &vec) {
            return !vec.empty() && vec.front().type == effect_type::ms_abigail;
        }, &character::responses);
        return it != m_characters.end()
            && (*it)->responses.front().get<effect_type::ms_abigail>().can_escape(origin, origin_card, flags);
    }
    
    void player::add_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !m_game->m_cubes.empty() && target->cubes.size() < 4; --ncubes) {
            int cube = m_game->m_cubes.back();
            m_game->m_cubes.pop_back();

            target->cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
        }
    }

    static void check_orange_card_empty(player *owner, card *target) {
        if (target->cubes.empty() && target->pile != card_pile_type::player_character) {
            owner->m_game->queue_event<event_type::delayed_action>([=]{
                owner->m_game->move_to(target, card_pile_type::discard_pile);
                owner->m_game->instant_event<event_type::post_discard_orange_card>(owner, target);
                if (!owner->m_game->table_cards_disabled(owner)) {
                    target->on_unequip(owner);
                }
            });
        }
    }

    void player::pay_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !target->cubes.empty(); --ncubes) {
            int cube = target->cubes.back();
            target->cubes.pop_back();

            m_game->m_cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, 0);
        }
        check_orange_card_empty(this, target);
    }

    void player::move_cubes(card *origin, card *target, int ncubes) {
        for(;ncubes!=0 && !origin->cubes.empty(); --ncubes) {
            int cube = origin->cubes.back();
            origin->cubes.pop_back();
            
            if (target->cubes.size() < 4) {
                target->cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
            } else {
                m_game->m_cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, 0);
            }
        }
        check_orange_card_empty(this, origin);
    }

    void player::drop_all_cubes(card *target) {
        for (int id : target->cubes) {
            m_game->m_cubes.push_back(id);
            m_game->add_public_update<game_update_type::move_cube>(id, 0);
        }
        target->cubes.clear();
    }

    void player::add_to_hand(card *target) {
        m_game->move_to(target, card_pile_type::player_hand, true, this);
    }

    static bool player_in_range(player *origin, player *target, int distance) {
        return origin->m_belltower > 0
            || origin->m_game->calc_distance(origin, target) <= distance;
    }

    void player::verify_modifiers(card *c, const std::vector<card *> &modifiers) {
        for (card *mod_card : modifiers) {
            card_suit_type suit = get_card_suit(mod_card);
            if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
                throw localized_game_error("ERROR_WRONG_DECLARED_SUIT");
            }

            if (mod_card->pile == card_pile_type::player_character && m_game->characters_disabled(this)) throw localized_game_error("ERROR_CHARACTERS_DISABLED");
            if (mod_card->pile == card_pile_type::player_table && m_game->table_cards_disabled(this)) throw localized_game_error("ERROR_TABLE_CARDS_DISABLED");
            if (mod_card->modifier != card_modifier_type::bangcard
                || std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::type) == c->effects.end()) {
                throw localized_game_error("ERROR_INVALID_ACTION");
            }
            for (const auto &e : mod_card->effects) {
                if (!e.can_play(mod_card, this)) {
                    throw localized_game_error("ERROR_INVALID_ACTION");
                }
            }
        }
    }

    void player::play_modifiers(const std::vector<card *> &modifiers) {
        for (card *mod_card : modifiers) {
            do_play_card(mod_card, false, std::vector{mod_card->effects.size(),
                play_card_target{enums::enum_constant<play_card_target_type::target_none>{}}});
        }
    }

    void player::verify_equip_target(card *c, const std::vector<play_card_target> &targets) {
        card_suit_type suit = get_card_suit(c);
        if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
            throw localized_game_error("ERROR_WRONG_DECLARED_SUIT");
        }

        if (targets.size() != 1) throw localized_game_error("ERROR_INVALID_ACTION");
        if (targets.front().enum_index() != play_card_target_type::target_player) throw localized_game_error("ERROR_INVALID_ACTION");
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (c->equips.empty()) return;
        if (tgts.size() != 1) throw localized_game_error("ERROR_INVALID_ACTION");
        player *target = m_game->get_player(tgts.front().player_id);
        if (!std::ranges::all_of(util::enum_flag_values(c->equips.front().target),
            [&](target_type value) {
                switch (value) {
                case target_type::player: return target->alive();
                case target_type::dead: return target->m_hp == 0;
                case target_type::self: return target->id == id;
                case target_type::notself: return target->id != id;
                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                case target_type::maxdistance: return player_in_range(this, target, c->equips.front().args + m_range_mod);
                default: return false;
                }
            })) throw localized_game_error("ERROR_INVALID_ACTION");
    }

    static bool is_new_target(const std::multimap<card *, player *> &targets, card *c, player *p) {
        auto [lower, upper] = targets.equal_range(c);
        return std::ranges::find(lower, upper, p, [](const auto &pair) { return pair.second; }) == upper;
    }

    void player::verify_card_targets(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        card_suit_type suit = get_card_suit(m_virtual ? m_virtual->corresponding_card : card_ptr);
        if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
            throw localized_game_error("ERROR_WRONG_DECLARED_SUIT");
        }

        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        if (card_ptr->cost > m_gold) throw localized_game_error("ERROR_NOT_ENOUGH_GOLD");
        if (card_ptr->max_usages != 0 && card_ptr->usages >= card_ptr->max_usages) throw localized_game_error("ERROR_INVALID_ACTION");

        int diff = targets.size() - effects.size();
        if (!card_ptr->optionals.empty() && card_ptr->optionals.back().is(effect_type::repeatable)) {
            if (diff % card_ptr->optionals.size() != 0) throw localized_game_error("ERROR_INVALID_TARGETS");
        } else {
            if (diff != 0 && diff != card_ptr->optionals.size()) throw localized_game_error("ERROR_INVALID_TARGETS");
        }
        if (!std::ranges::all_of(targets, [&, it = effects.begin(), end = effects.end()] (const play_card_target &target) mutable {
            const effect_holder &e = *it++;
            if (it == end) {
                it = card_ptr->optionals.begin();
                end = card_ptr->optionals.end();
            }
            return enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    if (!e.can_play(card_ptr, this)) return false;
                    return e.target == enums::flags_none<target_type>;
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    if (!bool(e.target & (target_type::player | target_type::dead))) return false;
                    if (bool(e.target & target_type::everyone)) {
                        std::vector<target_player_id> ids;
                        if (bool(e.target & target_type::notself)) {
                            for (auto *p = this;;) {
                                p = m_game->get_next_player(p);
                                if (p == this) break;
                                if (!e.can_play(card_ptr, this, p)) return false;
                                ids.emplace_back(p->id);
                            }
                        } else {
                            for (auto *p = this;;) {
                                ids.emplace_back(p->id);
                                if (!e.can_play(card_ptr, this, p)) return false;
                                p = m_game->get_next_player(p);
                                if (p == this) break;
                            }
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    } else if (args.size() != 1) {
                        return false;
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        if (!e.can_play(card_ptr, this, target)) return false;
                        return std::ranges::all_of(util::enum_flag_values(e.target),
                            [&](target_type value) {
                                switch (value) {
                                case target_type::player: return target->alive();
                                case target_type::dead: return !target->alive();
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, card_ptr, target);
                                case target_type::fanning_target: {
                                    player *prev_target = m_game->get_player(targets.front().get<play_card_target_type::target_player>().front().player_id);
                                    return player_in_range(prev_target, target, 1) && target != prev_target;
                                }
                                default: return false;
                                }
                            });
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    if (!bool(e.target & (target_type::card | target_type::cube_slot))) return false;
                    if (bool(e.target & target_type::everyone)) {
                        if (!std::ranges::all_of(m_game->m_players | std::views::filter(&player::alive), [&](const player &p) {
                            bool found = std::ranges::find(args, p.id, &target_card_id::player_id) != args.end();
                            if (p.m_hand.empty() && p.m_table.empty()) return !found;
                            if (bool(e.target & target_type::notself)) {
                                return (p.id == id) != found;
                            } else {
                                return found;
                            }
                        })) return false;
                        return std::ranges::all_of(args, [&](const target_card_id &arg) {
                            if (arg.player_id == id) return false;
                            auto *target = m_game->get_player(arg.player_id);
                            auto *card = m_game->find_card(arg.card_id);
                            if (!e.can_play(card_ptr, this, target, card)) return false;
                            return card->owner == target;
                        });
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        card *target_card = m_game->find_card(args.front().card_id);
                        if (!e.can_play(card_ptr, this, target, target_card)) return false;
                        return std::ranges::all_of(util::enum_flag_values(e.target),
                            [&](target_type value) {
                                switch (value) {
                                case target_type::card: return target->alive() && target_card->color != card_color_type::black;
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, card_ptr, target);
                                case target_type::table: return target_card->pile == card_pile_type::player_table;
                                case target_type::hand: return target_card->pile == card_pile_type::player_hand;
                                case target_type::blue: return target_card->color == card_color_type::blue;
                                case target_type::clubs: return get_card_suit(target_card) == card_suit_type::clubs;
                                case target_type::bang: 
                                    return !target_card->effects.empty() && target_card->effects.front().is(effect_type::bangcard);
                                case target_type::missed:
                                    return !target_card->responses.empty() && target_card->responses.front().is(effect_type::missedcard);
                                case target_type::bangormissed:
                                    if (!target_card->effects.empty()) return target_card->effects.front().is(effect_type::bangcard);
                                    else if (!target_card->responses.empty()) return target_card->responses.front().is(effect_type::missedcard);
                                    return false;
                                case target_type::cube_slot:
                                    return target_card == target->m_characters.front()
                                        || target_card->color == card_color_type::orange;
                                case target_type::can_repeat: return true;
                                default: return false;
                                }
                            });
                    }
                }
            }, target);
        })) throw localized_game_error("ERROR_INVALID_ACTION");
    }

    void player::do_play_card(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        auto play_card_action = [this, is_response](card *card_ptr) {
            switch (card_ptr->pile) {
            case card_pile_type::player_hand:
                m_game->move_to(card_ptr, card_pile_type::discard_pile);
                m_game->queue_event<event_type::on_play_hand_card>(this, card_ptr);
                [[fallthrough]];
            case card_pile_type::scenario_card:
            case card_pile_type::player_character:
                m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_CARD", card_ptr, this);
                break;
            case card_pile_type::player_table:
                if (card_ptr->color == card_color_type::green) {
                    m_game->move_to(card_ptr, card_pile_type::discard_pile);
                }
                m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_TABLE_CARD", card_ptr, this);
                break;
            case card_pile_type::shop_selection:
                if (card_ptr->color == card_color_type::brown) {
                    m_game->move_to(card_ptr, card_pile_type::shop_discard);
                }
                m_game->add_log(is_response ? "LOG_PLAYED_CARD" : "LOG_BOUGHT_CARD", card_ptr, this);
                break;
            }
        };

        assert(card_ptr != nullptr);
        if (m_virtual && card_ptr == &m_virtual->virtual_card) {
            play_card_action(m_virtual->corresponding_card);
            m_last_played_card = nullptr;
        } else {
            play_card_action(card_ptr);
            m_last_played_card = card_ptr;
        }

        if (card_ptr->max_usages != 0) {
            ++card_ptr->usages;
        }

        auto check_immunity = [&](player *target) {
            return target->immune_to(m_virtual ? m_virtual->corresponding_card : card_ptr);
        };

        int initial_belltower = m_belltower;
        
        if (card_ptr->cost > 0) {
            add_gold(-card_ptr->cost);
        }
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        auto effect_it = effects.begin();
        auto effect_end = effects.end();
        for (auto &t : targets) {
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    effect_it->on_play(card_ptr, this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    effect_it->flags |= effect_flags::single_target & static_cast<effect_flags>(-(args.size() == 1));
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        m_current_card_targets.emplace(card_ptr, p);
                        if (p != this && check_immunity(p)) {
                            if (effect_it->is(effect_type::bangcard)) {
                                request_bang req{card_ptr, this, p};
                                apply_bang_mods(req);
                                req.cleanup();
                            }
                        } else {
                            effect_it->on_play(card_ptr, this, p);
                        }
                    }
                    effect_it->flags &= ~effect_flags::single_target;
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    effect_it->flags |= effect_flags::single_target & static_cast<effect_flags>(-(args.size() == 1));
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        m_current_card_targets.emplace(card_ptr, p);
                        auto *target_card = m_game->find_card(target.card_id);
                        if (p != this && target_card->pile == card_pile_type::player_hand) {
                            effect_it->on_play(card_ptr, this, p, p->random_hand_card());
                        } else {
                            effect_it->on_play(card_ptr, this, p, target_card);
                        }
                    }
                    effect_it->flags &= ~effect_flags::single_target;
                }
            }, t);
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }
        }

        m_game->instant_event<event_type::on_play_card_end>(this, card_ptr);
        m_game->queue_event<event_type::on_effect_end>(this, card_ptr);

        if (m_virtual && card_ptr == &m_virtual->virtual_card) {
            m_virtual.reset();
        }
        
        if (m_belltower == initial_belltower) {
            m_belltower = 0;
        }
    }

    void player::play_card(const play_card_args &args) {
        std::vector<card *> modifiers;
        for (int id : args.modifier_ids) {
            modifiers.push_back(m_game->find_card(id));
        }

        if (m_virtual && args.card_id == m_virtual->virtual_card.id) {
            verify_modifiers(&m_virtual->virtual_card, modifiers);
            verify_card_targets(&m_virtual->virtual_card, false, args.targets);
            play_modifiers(modifiers);
            do_play_card(&m_virtual->virtual_card, false, args.targets);
        } else if (bool(args.flags & play_card_flags::sell_beer)) {
            if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
            if (!m_game->has_expansion(card_expansion_type::goldrush)
                || args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw localized_game_error("ERROR_INVALID_ACTION");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw localized_game_error("ERROR_INVALID_ACTION");
            card *target_card = m_game->find_card(l.front().card_id);
            if (target_card->effects.empty() || !target_card->effects.front().is(effect_type::beer)) throw localized_game_error("ERROR_INVALID_ACTION");
            discard_card(target_card);
            add_gold(1);
            m_game->add_log("LOG_SOLD_BEER", nullptr, this, nullptr, target_card);
            m_game->queue_event<event_type::on_play_beer>(this);
            m_game->queue_event<event_type::on_effect_end>(this, target_card);
        } else if (bool(args.flags & play_card_flags::discard_black)) {
            if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
            if (args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw localized_game_error("ERROR_INVALID_ACTION");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw localized_game_error("ERROR_INVALID_ACTION");
            auto *target_player = m_game->get_player(l.front().player_id);
            if (target_player == this) throw localized_game_error("ERROR_CANT_DISCARD_OWN_BLACK");
            card *target_card = m_game->find_card(l.front().card_id);
            if (target_card->color != card_color_type::black) throw localized_game_error("ERROR_INVALID_ACTION");
            if (m_gold < target_card->buy_cost + 1) throw localized_game_error("ERROR_NOT_ENOUGH_GOLD");
            add_gold(-target_card->buy_cost - 1);
            target_player->discard_card(target_card);
            m_game->add_log("LOG_DISCARDED_BLACK", nullptr, this, target_player, target_card);
        } else {
            card *card_ptr = m_game->find_card(args.card_id);
            switch(card_ptr->pile) {
            case card_pile_type::player_character:
                if (!card_ptr->effects.empty()) {
                    if (m_game->characters_disabled(this)) throw localized_game_error("ERROR_CHARACTERS_ARE_DISABLED");
                    if (!card_ptr->effects.empty() && card_ptr->effects.front().is(effect_type::drawing)) {
                        if (m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_NOT_DRAW");
                        verify_card_targets(card_ptr, false, args.targets);
                        m_game->add_private_update<game_update_type::status_clear>(this);
                        m_game->add_log("LOG_DRAWN_WITH_CHARACTER", card_ptr, this);
                    } else {
                        if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
                        verify_modifiers(card_ptr, modifiers);
                        verify_card_targets(card_ptr, false, args.targets);
                        m_game->add_log("LOG_PLAYED_CHARACTER", card_ptr, this);
                        play_modifiers(modifiers);
                    }
                    do_play_card(card_ptr, false, args.targets);
                }
                break;
            case card_pile_type::player_hand:
                if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
                switch (card_ptr->color) {
                case card_color_type::brown:
                    verify_modifiers(card_ptr, modifiers);
                    verify_card_targets(card_ptr, false, args.targets);
                    play_modifiers(modifiers);
                    do_play_card(card_ptr, false, args.targets);
                    break;
                case card_color_type::blue: {
                    if (m_game->has_scenario(scenario_flags::judge)) throw localized_game_error("ERROR_CANT_EQUIP_CARDS");
                    verify_equip_target(card_ptr, args.targets);
                    auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (target->has_card_equipped(card_ptr->name)) throw localized_game_error("ERROR_DUPLICATED_CARD");
                    target->equip_card(card_ptr);
                    if (this == target) {
                        m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, this);
                    } else {
                        m_game->add_log("LOG_EQUIPPED_CARD_TO", card_ptr, this, target);
                    }
                    if (m_game->has_expansion(card_expansion_type::armedanddangerous) && can_receive_cubes()) {
                        m_game->queue_request<request_type::add_cube>(0, nullptr, this);
                    }
                    m_game->queue_event<event_type::on_equip>(this, target, card_ptr);
                    m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                    break;
                }
                case card_color_type::green: {
                    if (m_game->has_scenario(scenario_flags::judge)) throw localized_game_error("ERROR_CANT_EQUIP_CARDS");
                    if (has_card_equipped(card_ptr->name)) throw localized_game_error("ERROR_DUPLICATED_CARD");
                    card_ptr->inactive = true;
                    equip_card(card_ptr);
                    m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, this);
                    m_game->queue_event<event_type::on_equip>(this, this, card_ptr);
                    m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                    m_game->add_public_update<game_update_type::tap_card>(card_ptr->id, true);
                    break;
                }
                case card_color_type::orange: {
                    if (m_game->has_scenario(scenario_flags::judge)) throw localized_game_error("ERROR_CANT_EQUIP_CARDS");
                    verify_equip_target(card_ptr, args.targets);
                    auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (target->has_card_equipped(card_ptr->name)) throw localized_game_error("ERROR_DUPLICATED_CARD");
                    if (m_game->m_cubes.size() < 3) throw localized_game_error("ERROR_NOT_ENOUGH_CUBES");
                    if (target->immune_to(card_ptr)) {
                        discard_card(card_ptr);
                    } else {
                        target->equip_card(card_ptr);
                        add_cubes(card_ptr, 3);
                        m_game->queue_event<event_type::on_equip>(this, target, card_ptr);
                        if (this == target) {
                            m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, this);
                        } else {
                            m_game->add_log("LOG_EQUIPPED_CARD_TO", card_ptr, this, target);
                        }
                    }
                    m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                }
                }
                break;
            case card_pile_type::player_table:
                if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
                if (m_game->table_cards_disabled(this)) throw localized_game_error("ERROR_TABLE_CARDS_ARE_DISABLED");
                if (card_ptr->inactive) throw localized_game_error("ERROR_CARD_INACTIVE");
                verify_modifiers(card_ptr, modifiers);
                verify_card_targets(card_ptr, false, args.targets);
                play_modifiers(modifiers);
                do_play_card(card_ptr, false, args.targets);
                break;
            case card_pile_type::shop_selection: {
                if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
                int discount = 0;
                if (!modifiers.empty()) {
                    if (auto modifier_it = std::ranges::find(m_characters, modifiers.front()->id, &character::id); modifier_it != m_characters.end()) {
                        if ((*modifier_it)->modifier == card_modifier_type::discount
                            && (*modifier_it)->usages < (*modifier_it)->max_usages) {
                            discount = 1;
                        } else {
                            throw localized_game_error("ERROR_INVALID_ACTION");
                        }
                    }
                }
                if (m_gold >= card_ptr->buy_cost - discount) {
                    switch (card_ptr->color) {
                    case card_color_type::brown:
                        verify_card_targets(card_ptr, false, args.targets);
                        play_modifiers(modifiers);
                        add_gold(discount - card_ptr->buy_cost);
                        do_play_card(card_ptr, false, args.targets);
                        m_game->queue_event<event_type::delayed_action>([this]{
                            while (m_game->m_shop_selection.size() < 3) {
                                m_game->draw_shop_card();
                            }
                        });
                    break;
                    case card_color_type::black:
                        if (m_game->has_scenario(scenario_flags::judge)) throw localized_game_error("ERROR_CANT_EQUIP_CARDS");
                        verify_equip_target(card_ptr, args.targets);
                        play_modifiers(modifiers);
                        auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                        if (target->has_card_equipped(card_ptr->name)) throw localized_game_error("ERROR_DUPLICATED_CARD");
                        add_gold(discount - card_ptr->buy_cost);
                        target->equip_card(card_ptr);
                        if (this == target) {
                            m_game->add_log("LOG_BOUGHT_EQUIP", card_ptr, this);
                        } else {
                            m_game->add_log("LOG_BOUGHT_EQUIP_TO", card_ptr, this, target);
                        }
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                        m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                        break;
                    }
                    break;
                } else {
                    throw localized_game_error("ERROR_NOT_ENOUGH_GOLD");
                }
            }
            case card_pile_type::scenario_card: {
                bool active_when_drawing = !card_ptr->effects.empty() && card_ptr->effects.front().is(effect_type::drawing);
                if (active_when_drawing == m_has_drawn) {
                    if (active_when_drawing) {
                        throw localized_game_error("ERROR_PLAYER_MUST_NOT_DRAW");
                    } else {
                        throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
                    }
                }
                verify_card_targets(card_ptr, false, args.targets);
                do_play_card(card_ptr, false, args.targets);
                break;
            }
            default:
                throw game_error("play_card: invalid card");
            }
        }
        m_start_of_turn = false;
    }
    
    void player::respond_card(const play_card_args &args) {
        card *card_ptr = m_game->find_card(args.card_id);

        if (std::ranges::none_of(card_ptr->responses, [=, this](const effect_holder &e){
            return e.can_respond(card_ptr, this);
        })) return;

        switch (card_ptr->pile) {
        case card_pile_type::player_character:
            if (m_game->characters_disabled(this)) throw localized_game_error("ERROR_CHARACTERS_ARE_DISABLED");
            break;
        case card_pile_type::player_table:
            if (m_game->table_cards_disabled(this)) throw localized_game_error("ERROR_TABLE_CARDS_ARE_DISABLED");
            if (card_ptr->inactive) throw localized_game_error("ERROR_CARD_INACTIVE");
            break;
        case card_pile_type::shop_selection:
            // hack per bottiglia e complice
            if (!m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_DRAW");
            break;
        case card_pile_type::player_hand:
            if (card_ptr->color != card_color_type::brown) return;
            break;
        default:
            throw game_error("respond_card: invalid card");
        }
        
        verify_card_targets(card_ptr, true, args.targets);
        do_play_card(card_ptr, true, args.targets);
    }

    void player::draw_from_deck() {
        if (m_has_drawn) throw localized_game_error("ERROR_PLAYER_MUST_NOT_DRAW");
        int save_numcards = m_num_cards_to_draw;
        m_game->queue_event<event_type::on_draw_from_deck>(this);
        if (!m_has_drawn) {
            while (m_num_drawn_cards<m_num_cards_to_draw) {
                ++m_num_drawn_cards;
                card *drawn_card = m_game->draw_phase_one_card_to(card_pile_type::player_hand, this);
                m_game->instant_event<event_type::on_card_drawn>(this, drawn_card);
            }
            m_game->add_log("LOG_DRAWN_FROM_DECK", nullptr, this);
        }
        m_num_cards_to_draw = save_numcards;
        m_has_drawn = true;
        m_game->queue_event<event_type::post_draw_cards>(this);
        if (m_game->m_requests.empty()) {
            m_game->add_private_update<game_update_type::status_clear>(this);
        }
    }

    void player::play_virtual_card(card *corresponding_card, card virtual_card) {
        virtual_card_update obj;
        obj.virtual_id = m_game->get_next_id();
        obj.card_id = corresponding_card->id;
        obj.color = virtual_card.color;
        obj.suit = virtual_card.suit;
        obj.value = virtual_card.value;
        for (const auto &value : virtual_card.effects) {
            card_target_data ctd;
            ctd.type = value.type;
            ctd.target = value.target;
            ctd.args = value.args;
            obj.targets.emplace_back(std::move(ctd));
        }

        virtual_card.id = obj.virtual_id;
        m_virtual.emplace(corresponding_card, std::move(virtual_card));

        m_game->add_private_update<game_update_type::virtual_card>(this, obj);
    }

    card_suit_type player::get_card_suit(card *drawn_card) {
        card_suit_type suit = drawn_card->suit;
        m_game->instant_event<event_type::apply_suit_modifier>(suit);
        return suit;
    }

    card_value_type player::get_card_value(card *drawn_card) {
        card_value_type value = drawn_card->value;
        m_game->instant_event<event_type::apply_value_modifier>(value);
        return value;
    }

    void player::start_of_turn() {
        if (this != m_game->m_playing && this == m_game->m_first_player) {
            m_game->queue_event<event_type::delayed_action>([&]{
                m_game->draw_scenario_card();
            });
        }
        
        m_game->m_playing = this;

        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        m_has_drawn = false;
        m_start_of_turn = true;
        m_declared_suit = card_suit_type::none;

        if (!m_ghost && m_hp == 0) {
            if (m_game->has_scenario(scenario_flags::ghosttown)) {
                ++m_num_cards_to_draw;
                for (auto *c : m_characters) {
                    c->on_equip(this);
                }
            } else if (m_game->has_scenario(scenario_flags::deadman) && this == m_game->m_first_dead) {
                m_dead = false;
                m_game->add_public_update<game_update_type::player_hp>(id, m_hp = 2);
                m_game->draw_card_to(card_pile_type::player_hand, this);
                m_game->draw_card_to(card_pile_type::player_hand, this);
                for (auto *c : m_characters) {
                    c->on_equip(this);
                }
            }
        }
        
        for (character *c : m_characters) {
            c->usages = 0;
        }
        for (card *c : m_table) {
            c->usages = 0;
        }

        m_current_card_targets.clear();
        
        m_game->add_public_update<game_update_type::switch_turn>(id);
        m_game->add_log("LOG_TURN_START", nullptr, this);
        m_game->queue_event<event_type::pre_turn_start>(this);
        m_game->queue_event<event_type::delayed_action>([&]{
            if (m_predraw_checks.empty()) {
                m_game->queue_event<event_type::on_turn_start>(this);
            } else {
                for (auto &[card_id, obj] : m_predraw_checks) {
                    obj.resolved = false;
                }
                m_game->queue_request<request_type::predraw>(0, this, this);
            }
        });
    }

    player::predraw_check *player::get_if_top_predraw_check(card *target_card) {
        int top_priority = std::ranges::max(m_predraw_checks
            | std::views::values
            | std::views::filter(std::not_fn(&predraw_check::resolved))
            | std::views::transform(&predraw_check::priority));
        auto it = m_predraw_checks.find(target_card);
        if (it != m_predraw_checks.end()
            && !it->second.resolved
            && it->second.priority == top_priority) {
            return &it->second;
        }
        return nullptr;
    }

    void player::next_predraw_check(card *target_card) {
        m_game->queue_event<event_type::delayed_action>([this, target_card]{
            if (auto it = m_predraw_checks.find(target_card); it != m_predraw_checks.end()) {
                it->second.resolved = true;
            }

            if (std::ranges::all_of(m_predraw_checks | std::views::values, &predraw_check::resolved)) {
                m_game->queue_event<event_type::on_turn_start>(this);
            } else {
                m_game->queue_request<request_type::predraw>(nullptr, this, this);
            }
        });
    }

    void player::pass_turn(player *next_player) {
        if (num_hand_cards() > max_cards_end_of_turn()) {
            m_game->queue_request<request_type::discard_pass>(nullptr, this, this);
        } else {
            end_of_turn(next_player);
        }
    }

    void player::end_of_turn(player *next_player) {
        for (card *c : m_table) {
            if (c->inactive) {
                c->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c->id, false);
            }
        }
        
        m_current_card_targets.clear();

        m_game->m_ignore_next_turn = false;
        m_game->instant_event<event_type::on_turn_end>(this);
        if (m_game->num_alive() > 0) {
            if (!m_game->m_ignore_next_turn) {
                if (!next_player) {
                    if (!m_ghost && m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown)) {
                        --m_num_cards_to_draw;
                        m_game->player_death(this);
                    }
                    next_player = m_game->get_next_in_turn(this);
                }
                next_player->start_of_turn();
            }
        }
    }

    void player::discard_all() {
        while (!m_table.empty()) {
            discard_card(m_table.front());
        }
        drop_all_cubes(m_characters.front());
        while (!m_hand.empty()) {
            m_game->move_to(m_hand.front(), card_pile_type::discard_pile);
        }
    }

    void player::set_character_and_role(character *c, player_role role) {
        m_role = role;

        m_max_hp = c->max_hp;
        if (role == player_role::sheriff) {
            ++m_max_hp;
        }
        m_hp = m_max_hp;

        m_characters.emplace_back(c);
        c->pile = card_pile_type::player_character;
        c->owner = this;
        c->on_equip(this);
        m_game->send_character_update(*c, id, 0);

        if (role == player_role::sheriff || m_game->m_players.size() <= 3) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role, true);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role, true);
        }
    }
}