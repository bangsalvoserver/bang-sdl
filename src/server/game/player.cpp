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
        target->on_equip(this);
        m_game->move_to(target, card_pile_type::player_table, true, this, show_card_flags::show_everyone);
    }

    bool player::has_card_equipped(const std::string &name) const {
        return std::ranges::find(m_table, name, &card::name) != m_table.end();
    }

    card *player::random_hand_card() {
        return m_hand[std::uniform_int_distribution<int>(0, m_hand.size() - 1)(m_game->rng)];
    }

    std::vector<card *>::iterator player::move_card_to(card *target_card, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (target_card->location == &m_table) {
            if (target_card->inactive) {
                target_card->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(target_card->id, false);
            }
            drop_all_cubes(target_card);
            auto it = m_game->move_to(target_card, pile, known, owner, flags);
            m_game->queue_event<event_type::post_discard_card>(this, target_card);
            target_card->on_unequip(this);
            return it;
        } else if (target_card->location == &m_hand) {
            return m_game->move_to(target_card, pile, known, owner, flags);
        } else {
            throw game_error("Carta non trovata");
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
        if (!m_ghost) {
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
        if (m_hp <= 0) {
            m_game->add_request<request_type::death>(origin_card, origin, this);
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
        if (m_characters.front()->cubes.size() < 4) return true;
        return std::ranges::any_of(m_table, [](const card *card) {
            return card->color == card_color_type::orange && card->cubes.size() < 4;
        });
    }

    bool player::can_escape(card *origin_card, effect_flags flags) const {
        // hack per determinare se e' necessario attivare request di fuga
        if (bool(flags & effect_flags::escapable)
            && m_game->has_expansion(card_expansion_type::valleyofshadows)) return true;
        auto it = std::ranges::find_if(m_characters, [](const auto &vec) {
            return !vec.empty() && vec.front().enum_index() == effect_type::ms_abigail;
        }, &character::responses);
        return it != m_characters.end()
            && (*it)->responses.front().get<effect_type::ms_abigail>().can_escape(this, origin_card, flags);
    }
    
    void player::add_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !m_game->m_cubes.empty() && target->cubes.size() < 4; --ncubes) {
            int cube = m_game->m_cubes.back();
            m_game->m_cubes.pop_back();

            target->cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
        }
    }

    void player::pay_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !target->cubes.empty(); --ncubes) {
            int cube = target->cubes.back();
            target->cubes.pop_back();

            m_game->m_cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, 0);
        }
        if (target->cubes.empty() && target != m_characters.front()) {
            m_game->queue_event<event_type::delayed_action>([this, target]{
                m_game->move_to(target, card_pile_type::discard_pile);
                m_game->instant_event<event_type::post_discard_orange_card>(this, target);
                target->on_unequip(this);
            });
        }
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
        if (origin->cubes.empty() && origin != m_characters.front()) {
            m_game->queue_event<event_type::delayed_action>([this, origin]{
                m_game->move_to(origin, card_pile_type::discard_pile);
                m_game->instant_event<event_type::post_discard_orange_card>(this, origin);
                origin->on_unequip(this);
            });
        }
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
            if (mod_card->modifier != card_modifier_type::bangcard
                || std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::enum_index) == c->effects.end()) {
                throw game_error("Azione non valida");
            }
            for (const auto &e : mod_card->effects) {
                if (!e.can_play(mod_card, this)) {
                    throw game_error("Azione non valida");
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
        if (targets.size() != 1) throw game_error("Azione non valida");
        if (targets.front().enum_index() != play_card_target_type::target_player) throw game_error("Azione non valida");
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (c->equips.empty()) return;
        if (tgts.size() != 1) throw game_error("Azione non valida");
        player *target = m_game->get_player(tgts.front().player_id);
        if (!std::ranges::all_of(util::enum_flag_values(c->equips.front().target()),
            [&](target_type value) {
                switch (value) {
                case target_type::player: return target->alive();
                case target_type::dead: return !target->alive();
                case target_type::self: return target->id == id;
                case target_type::notself: return target->id != id;
                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                case target_type::maxdistance: return player_in_range(this, target, c->equips.front().args() + m_range_mod);
                default: return false;
                }
            })) throw game_error("Azione non valida");
    }

    static bool is_new_target(const std::multimap<card *, player *> &targets, card *c, player *p) {
        auto [lower, upper] = targets.equal_range(c);
        return std::ranges::find(lower, upper, p, [](const auto &pair) { return pair.second; }) == upper;
    }

    void player::verify_card_targets(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        if (card_ptr->cost > m_gold) throw game_error("Non hai abbastanza pepite");
        if (card_ptr->max_usages != 0 && card_ptr->usages >= card_ptr->max_usages) throw game_error("Azione non valida");

        int diff = targets.size() - effects.size();
        if (card_ptr->optional_repeatable) {
            if (diff % card_ptr->optionals.size() != 0) throw game_error("Target non validi");
        } else {
            if (diff != 0 && diff != card_ptr->optionals.size()) throw game_error("Target non validi");
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
                        return std::ranges::all_of(util::enum_flag_values(e.target()),
                            [&](target_type value) {
                                switch (value) {
                                case target_type::player: return target->alive();
                                case target_type::dead: return !target->alive();
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args() + m_range_mod);
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
                    if (!bool(e.target() & (target_type::card | target_type::cube_slot))) return false;
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
                            auto *card = m_game->find_card(arg.card_id);
                            if (!e.can_play(card_ptr, this, target, card)) return false;
                            return card->location == &target->m_hand || card->location == &target->m_table;
                        });
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        card *target_card = m_game->find_card(args.front().card_id);
                        if (!e.can_play(card_ptr, this, target, target_card)) return false;
                        return std::ranges::all_of(util::enum_flag_values(e.target()),
                            [&](target_type value) {
                                switch (value) {
                                case target_type::card: return target->alive() && target_card->color != card_color_type::black;
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.args() + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, card_ptr, target);
                                case target_type::table: return target_card->location == &target->m_table;
                                case target_type::hand: return target_card->location == &target->m_hand;
                                case target_type::blue: return target_card->color == card_color_type::blue;
                                case target_type::clubs: return target_card->suit == card_suit_type::clubs;
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
        })) throw game_error("Azione non valida");
    }

    void player::do_play_card(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        if (card_ptr->location == &m_hand) {
            m_last_played_card = card_ptr;

            m_game->move_to(card_ptr, card_pile_type::discard_pile);

            m_game->queue_event<event_type::on_play_hand_card>(this, card_ptr);
        } else if (card_ptr->location == &m_table) {
            m_last_played_card = card_ptr;

            if (card_ptr->color == card_color_type::green) {
                m_game->move_to(card_ptr, card_pile_type::discard_pile);
            }
        } else if (card_ptr->location == &m_game->m_shop_selection) {
            if (card_ptr->color == card_color_type::brown) {
                m_last_played_card = card_ptr;

                m_game->move_to(card_ptr, card_pile_type::shop_discard);
            }
        } else if (m_virtual && card_ptr == &m_virtual->virtual_card) {
            m_last_played_card = nullptr;

            m_game->move_to(m_virtual->corresponding_card, card_pile_type::discard_pile);
            m_game->queue_event<event_type::on_play_hand_card>(this, m_virtual->corresponding_card);
        }

        assert(card_ptr != nullptr);

        if (card_ptr->max_usages != 0) {
            ++card_ptr->usages;
        }

        auto check_immunity = [&](player *target) {
            return target->immune_to(*(m_virtual ? m_virtual->corresponding_card : card_ptr));
        };

        int initial_belltower = m_belltower;
        
        if (card_ptr->cost > 0) {
            add_gold(-card_ptr->cost);
        }
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        auto effect_it = effects.begin();
        auto effect_end = effects.end();
        for (auto &t : targets) {
            effect_it->flags() |= effect_flags::single_target & static_cast<effect_flags>(-(targets.size() == 1));
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    effect_it->on_play(card_ptr, this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
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
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        m_current_card_targets.emplace(card_ptr, p);
                        auto *target_card = m_game->find_card(target.card_id);
                        if (p != this && target_card->location == &p->m_hand) {
                            effect_it->on_play(card_ptr, this, p, p->random_hand_card());
                        } else {
                            effect_it->on_play(card_ptr, this, p, target_card);
                        }
                    }
                }
            }, t);
            effect_it->flags() &= ~effect_flags::single_target;
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
            return;
        }

        card *card_ptr = args.card_id ? m_game->find_card(args.card_id) : nullptr;

        if (bool(args.flags & play_card_flags::sell_beer)) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (!m_game->has_expansion(card_expansion_type::goldrush)
                || args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw game_error("Azione non valida");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw game_error("Azione non valida");
            card *target_card = m_game->find_card(l.front().card_id);
            if (target_card->effects.empty() || !target_card->effects.front().is(effect_type::beer)) throw game_error("Azione non valida");
            m_game->add_log(this, nullptr, std::string("venduto ") + target_card->name);
            discard_card(target_card);
            add_gold(1);
            m_game->queue_event<event_type::on_play_beer>(this);
            m_game->queue_event<event_type::on_effect_end>(this, target_card);
        } else if (bool(args.flags & play_card_flags::discard_black)) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (args.targets.size() != 1
                || !args.targets.front().is(play_card_target_type::target_card)) throw game_error("Azione non valida");
            const auto &l = args.targets.front().get<play_card_target_type::target_card>();
            if (l.size() != 1) throw game_error("Azione non valida");
            auto *target_player = m_game->get_player(l.front().player_id);
            if (target_player == this) throw game_error("Non puoi scartare le tue carte nere");
            card *target_card = m_game->find_card(l.front().card_id);
            if (target_card->color != card_color_type::black) throw game_error("Azione non valida");
            if (m_gold < target_card->buy_cost + 1) throw game_error("Non hai abbastanza pepite");
            m_game->add_log(this, target_player, std::string("scartato ") + target_card->name);
            add_gold(-target_card->buy_cost - 1);
            target_player->discard_card(target_card);
        } else if (std::ranges::find(m_characters, card_ptr) != m_characters.end()) {
            switch (static_cast<character *>(card_ptr)->type) {
            case character_type::active:
                if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
                verify_modifiers(card_ptr, modifiers);
                verify_card_targets(card_ptr, false, args.targets);
                play_modifiers(modifiers);
                m_game->add_log(this, nullptr, std::string("giocato effetto di ") + card_ptr->name);
                do_play_card(card_ptr, false, args.targets);
                break;
            case character_type::drawing:
            case character_type::drawing_forced:
                if (m_num_drawn_cards >= m_num_cards_to_draw) throw game_error("Non devi pescare adesso");
                verify_card_targets(card_ptr, false, args.targets);
                m_game->add_private_update<game_update_type::status_clear>(this);
                m_game->add_log(this, nullptr, std::string("pescato con l'effetto di ") + card_ptr->name);
                do_play_card(card_ptr, false, args.targets);
                break;
            default:
                break;
            }
        } else if (card_ptr->location == &m_hand) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            switch (card_ptr->color) {
            case card_color_type::brown:
                verify_modifiers(card_ptr, modifiers);
                verify_card_targets(card_ptr, false, args.targets);
                play_modifiers(modifiers);
                m_game->add_log(this, nullptr, std::string("giocato ") + card_ptr->name);
                do_play_card(card_ptr, false, args.targets);
                break;
            case card_color_type::blue: {
                verify_equip_target(card_ptr, args.targets);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                if (target->has_card_equipped(card_ptr->name)) throw game_error("Carta duplicata");
                m_game->add_log(this, target, std::string("equipaggiato ") + card_ptr->name);
                target->equip_card(card_ptr);
                if (m_game->has_expansion(card_expansion_type::armedanddangerous) && can_receive_cubes()) {
                    m_game->queue_request<request_type::add_cube>(0, nullptr, this);
                }
                m_game->queue_event<event_type::on_equip>(this, target, card_ptr);
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                break;
            }
            case card_color_type::green: {
                if (has_card_equipped(card_ptr->name)) throw game_error("Carta duplicata");
                m_game->add_log(this, nullptr, std::string("equipaggiato ") + card_ptr->name);
                card_ptr->inactive = true;
                equip_card(card_ptr);
                m_game->queue_event<event_type::on_equip>(this, this, card_ptr);
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
                m_game->add_public_update<game_update_type::tap_card>(card_ptr->id, true);
                break;
            }
            case card_color_type::orange: {
                verify_equip_target(card_ptr, args.targets);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                if (target->has_card_equipped(card_ptr->name)) throw game_error("Carta duplicata");
                if (m_game->m_cubes.size() < 3) throw game_error("Non ci sono abbastanza cubetti");
                m_game->add_log(this, target, std::string("equipaggiato ") + card_ptr->name);
                target->equip_card(card_ptr);
                add_cubes(card_ptr, 3);
                m_game->queue_event<event_type::on_equip>(this, target, card_ptr);
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
            }
            }
        } else if (card_ptr->location == &m_table) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            if (card_ptr->inactive) throw game_error("Carta non attiva in questo turno");
            verify_modifiers(card_ptr, modifiers);
            verify_card_targets(card_ptr, false, args.targets);
            play_modifiers(modifiers);
            m_game->add_log(this, nullptr, std::string("giocato ") + card_ptr->name + " da terra");
            do_play_card(card_ptr, false, args.targets);
        } else if (card_ptr->location == &m_game->m_shop_selection) {
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            int discount = 0;
            if (!modifiers.empty()) {
                if (auto modifier_it = std::ranges::find(m_characters, modifiers.front()->id, &character::id); modifier_it != m_characters.end()) {
                    if ((*modifier_it)->modifier == card_modifier_type::discount
                        && (*modifier_it)->usages < (*modifier_it)->max_usages) {
                        discount = 1;
                    } else {
                        throw game_error("Azione non valida");
                    }
                }
            }
            if (m_gold >= card_ptr->buy_cost - discount) {
                switch (card_ptr->color) {
                case card_color_type::brown:
                    verify_card_targets(card_ptr, false, args.targets);
                    play_modifiers(modifiers);
                    add_gold(discount - card_ptr->buy_cost);
                    m_game->add_log(this, nullptr, std::string("comprato e giocato ") + card_ptr->name);
                    do_play_card(card_ptr, false, args.targets);
                    m_game->queue_event<event_type::delayed_action>([this]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                break;
                case card_color_type::black:
                    verify_equip_target(card_ptr, args.targets);
                    play_modifiers(modifiers);
                    auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (target->has_card_equipped(card_ptr->name)) throw game_error("Carta duplicata");
                    m_game->add_log(this, target, std::string("comprato e equipaggiato ") + card_ptr->name);
                    add_gold(discount - card_ptr->buy_cost);
                    target->equip_card(card_ptr);
                    while (m_game->m_shop_selection.size() < 3) {
                        m_game->draw_shop_card();
                    }
                    m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
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
        card *card_ptr = m_game->find_card(args.card_id);

        auto can_respond = [&] {
            return std::ranges::any_of(card_ptr->responses, [&](const effect_holder &e) {
                return e.can_respond(card_ptr, this);
            });
        };

        if (std::ranges::find(m_characters, card_ptr) != m_characters.end()) {
            if (m_game->m_characters_disabled > 0 && m_game->m_playing != this) throw game_error("I personaggi sono disabilitati");
            if (can_respond()) {
                verify_card_targets(card_ptr, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con l'effetto di ") + card_ptr->name);
                do_play_card(card_ptr, true, args.targets);
            }
        } else if (card_ptr->location == &m_hand) {
            if (card_ptr->color == card_color_type::brown && can_respond()) {
                verify_card_targets(card_ptr, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con ") + card_ptr->name);
                do_play_card(card_ptr, true, args.targets);
            }
        } else if (card_ptr->location == &m_table) {
            if (m_game->m_table_cards_disabled > 0 && m_game->m_playing != this) throw game_error("Le carte in gioco sono disabilitate");
            if (card_ptr->inactive) throw game_error("Carta non attiva in questo turno");
            if (can_respond()) {
                verify_card_targets(card_ptr, true, args.targets);
                m_game->add_log(this, nullptr, std::string("risposto con ") + card_ptr->name + " da terra");
                do_play_card(card_ptr, true, args.targets);
            }
        } else if (card_ptr->location == &m_game->m_shop_selection) {
            // hack per bottiglia e complice
            if (m_num_drawn_cards < m_num_cards_to_draw) throw game_error("Devi pescare");
            verify_card_targets(card_ptr, true, args.targets);
            m_game->add_log(this, nullptr, std::string("scelto opzione ") + card_ptr->name);
            do_play_card(card_ptr, true, args.targets);
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

    void player::play_virtual_card(card *corresponding_card, card virtual_card) {
        virtual_card_update obj;
        obj.virtual_id = m_game->get_next_id();
        obj.card_id = corresponding_card->id;
        obj.color = virtual_card.color;
        obj.suit = virtual_card.suit;
        obj.value = virtual_card.value;
        for (const auto &value : virtual_card.effects) {
            card_target_data ctd;
            ctd.type = value.enum_index();
            ctd.target = value.target();
            ctd.args = value.args();
            obj.targets.emplace_back(std::move(ctd));
        }

        virtual_card.id = obj.virtual_id;
        m_virtual.emplace(corresponding_card, std::move(virtual_card));

        m_game->add_private_update<game_update_type::virtual_card>(this, obj);
    }

    void player::start_of_turn() {
        m_game->m_next_turn = nullptr;
        m_game->m_playing = this;

        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        for (character *c : m_characters) {
            c->usages = 0;
        }
        for (card *c : m_table) {
            c->usages = 0;
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

    void player::pass_turn() {
        if (num_hand_cards() > max_cards_end_of_turn()) {
            m_game->queue_request<request_type::discard_pass>(nullptr, this, this);
        } else {
            end_of_turn();
        }
    }

    void player::end_of_turn() {
        for (card *c : m_table) {
            if (c->inactive) {
                c->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c->id, false);
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
        c->on_equip(this);
        m_game->send_character_update(*c, id, 0);

        if (role == player_role::sheriff || m_game->m_players.size() <= 3) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role, true);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role, true);
        }
    }
}