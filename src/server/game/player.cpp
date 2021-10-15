#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/requests.h"
#include "common/net_enums.h"

namespace banggame {
    using namespace enums::flag_operators;
    
    void player::discard_weapon(int card_id) {
        auto it = std::ranges::find_if(m_table, [card_id](const deck_card &c) {
            return !c.equips.empty() && c.equips.front().is(equip_type::weapon) && c.id != card_id;
        });
        if (it != m_table.end()) {
            m_game->add_to_discards(std::move(*it)).on_unequip(this);
            m_table.erase(it);
        }
    }

    void player::equip_card(deck_card &&target) {
        target.on_equip(this);
        auto &moved = m_table.emplace_back(std::move(target));
        m_game->send_card_update(moved);
        m_game->add_public_update<game_update_type::move_card>(moved.id, id, card_pile_type::player_table);
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
            auto &moved = m_game->add_to_discards(std::move(*it));
            m_table.erase(it);
            m_game->queue_event<event_type::post_discard_card>(this, card_id);
            moved.on_unequip(this);
            return moved;
        } else if (auto it = std::ranges::find(m_hand, card_id, &card::id); it != m_hand.end()) {
            auto &moved = m_game->add_to_discards(std::move(*it));
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
            auto &moved = add_to_hand(std::move(*it));
            target->m_table.erase(it);
            m_game->queue_event<event_type::post_discard_card>(this, card_id);
            moved.on_unequip(target);
            return moved;
        } else if (auto it = std::ranges::find(target->m_hand, card_id, &card::id); it != target->m_hand.end()) {
            auto &moved = add_to_hand(std::move(*it));
            target->m_hand.erase(it);
            m_game->queue_event<event_type::post_discard_card>(this, card_id);
            return moved;
        } else {
            throw game_error("server.steal_card: ID non trovato");
        }
    }

    void player::damage(int origin_card_id, player *source, int value, bool is_bang) {
        if (!m_ghost) {
            auto &obj = m_game->queue_request<request_type::damaging>(origin_card_id, source, this);
            obj.damage = value;
            obj.is_bang = is_bang;
        }
    }

    void player::heal(int value) {
        if (!m_ghost) {
            m_hp = std::min(m_hp + value, m_max_hp);
            m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        }
    }

    deck_card &player::add_to_hand(deck_card &&target) {
        auto &moved = m_hand.emplace_back(std::move(target));
        m_game->send_card_update(moved, this);
        m_game->add_public_update<game_update_type::move_card>(moved.id, id, card_pile_type::player_hand);
        return moved;
    }

    static bool player_in_range(player *origin, player *target, int distance) {
        return origin->m_game->calc_distance(origin, target) <= distance;
    }

    bool player::verify_equip_target(const card &c, const std::vector<play_card_target> &targets) {
        if (targets.size() != 1) return false;
        if (targets.front().enum_index() != play_card_target_type::target_player) return false;
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (c.equips.empty()) return true;
        if (tgts.size() != 1) return false;
        player *target = m_game->get_player(tgts.front().player_id);
        return std::ranges::all_of(enums::enum_values_v<target_type>
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
                case target_type::maxdistance: return player_in_range(this, target, c.equips.front().maxdistance() + m_range_mod);
                default: return false;
                }
            });
    }

    static bool is_new_target(const std::multimap<int, int> &targets, int card_id, int player_id) {
        auto [lower, upper] = targets.equal_range(card_id);
        return std::ranges::find(lower, upper, player_id, [](const auto &pair) { return pair.second; }) == upper;
    }

    bool player::verify_card_targets(const card &c, bool is_response, const std::vector<play_card_target> &targets) {
        auto &effects = is_response ? c.responses : c.effects;
        if (!std::ranges::all_of(effects, [this](const effect_holder &e) {
            return e.can_play(this);
        })) return false;

        if (effects.size() != targets.size()) return false;
        return std::ranges::all_of(effects, [&, it = targets.begin()] (const effect_holder &e) mutable {
            return enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
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
                                ids.emplace_back(p->id);
                            }
                        } else {
                            for (auto *p = this;;) {
                                ids.emplace_back(p->id);
                                p = m_game->get_next_player(p);
                                if (p == this) break;
                            }
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    } else if (args.size() != 1) {
                        return false;
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
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
                                case target_type::maxdistance: return player_in_range(this, target, e.maxdistance() + m_range_mod);
                                case target_type::attacker: return !m_game->m_requests.empty() && m_game->top_request().origin() == target;
                                case target_type::new_target: return is_new_target(m_current_card_targets, c.id, target->id);
                                case target_type::fanning_target: {
                                    player *prev_target = m_game->get_player((it - 2)->get<play_card_target_type::target_player>().front().player_id);
                                    return player_in_range(prev_target, target, 1);
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
                            if (arg.from_hand) return true;
                            auto &l = m_game->get_player(arg.player_id)->m_table;
                            return std::ranges::find(l, arg.card_id, &deck_card::id) != l.end();
                        });
                    } else {
                        player *target = m_game->get_player(args.front().player_id);
                        return std::ranges::all_of(enums::enum_values_v<target_type>
                            | std::views::filter([type = e.target()](target_type value) {
                                return bool(type & value);
                            }), [&](target_type value) {
                                switch (value) {
                                case target_type::card: return target->alive();
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return player_in_range(this, target, m_weapon_range + m_range_mod);
                                case target_type::maxdistance: return player_in_range(this, target, e.maxdistance() + m_range_mod);
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
                                default: return false;
                                }
                            });
                    }
                }
            }, *it++);
        });
    }

    void player::do_play_card(int card_id, bool is_response, const std::vector<play_card_target> &targets) {
        card *card_ptr = nullptr;
        bool is_character = false;
        bool is_virtual = false;
        if (auto it = std::ranges::find(m_characters, card_id, &character::id); it != m_characters.end()) {
            card_ptr = &*it;
            is_character = true;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            auto &moved = m_game->add_to_discards(std::move(*it));
            card_ptr = &moved;
            m_last_played_card = card_id;
            m_hand.erase(it);
            m_game->queue_event<event_type::on_play_hand_card>(this, card_id);
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            if (!m_game->table_cards_disabled(id)) {
                m_last_played_card = card_id;
                if (it->color == card_color_type::blue) {
                    card_ptr = &*it;
                } else {
                    auto &moved = m_game->add_to_discards(std::move(*it));
                    card_ptr = &moved;
                    m_table.erase(it);
                }
            } else {
                throw invalid_action();
            }
        } else if (m_virtual && card_id == m_virtual->first) {
            card_ptr = &m_virtual->second;
            is_virtual = true;
            m_last_played_card = 0;
            if (auto it = std::ranges::find(m_hand, m_virtual->second.id, &card::id); it != m_hand.end()) {
                m_game->add_to_discards(std::move(*it));
                m_hand.erase(it);
                m_game->queue_event<event_type::on_play_hand_card>(this, m_virtual->second.id);
            }
        } else {
            throw game_error("server.do_play_card: ID non trovato");
        }

        auto check_immunity = [&](player *target) {
            if (m_virtual) {
                return target->immune_to(m_virtual->second);
            }
            if (is_character) return false;
            return target->immune_to(*static_cast<deck_card*>(card_ptr));
        };

        size_t initial_mods_size = m_bang_mods.size();
        
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
        if (auto card_it = std::ranges::find(m_characters, args.card_id, &character::id); card_it != m_characters.end()) {
            switch (card_it->type) {
            case character_type::active:
                if (m_num_drawn_cards == m_num_cards_to_draw && (card_it->max_usages == 0 || card_it->usages < card_it->max_usages)) {
                    if (verify_card_targets(*card_it, false, args.targets)) {
                        do_play_card(args.card_id, false, args.targets);
                        ++card_it->usages;
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            case character_type::drawing:
            case character_type::drawing_forced:
                if (m_num_drawn_cards != m_num_cards_to_draw && (card_it->max_usages == 0) || card_it->usages < card_it->max_usages) {
                    if (verify_card_targets(*card_it, false, args.targets)) {
                        m_game->add_private_update<game_update_type::status_clear>(this);
                        do_play_card(args.card_id, false, args.targets);
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            default:
                break;
            }
        } else if (m_virtual && args.card_id == m_virtual->first) {
            if (verify_card_targets(m_virtual->second, false, args.targets)) {
                do_play_card(args.card_id, false, args.targets);
            } else {
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            if (m_num_drawn_cards != m_num_cards_to_draw) return;
            switch (card_it->color) {
            case card_color_type::brown:
                if (verify_card_targets(*card_it, false, args.targets)) {
                    do_play_card(args.card_id, false, args.targets);
                } else {
                    throw invalid_action();
                }
                break;
            case card_color_type::blue:
                if (verify_equip_target(*card_it, args.targets)) {
                    auto *target = m_game->get_player(
                        args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (!target->has_card_equipped(card_it->name)) {
                        deck_card removed = std::move(*card_it);
                        m_hand.erase(card_it);
                        target->equip_card(std::move(removed));
                        m_game->queue_event<event_type::on_equip>(this, target, args.card_id);
                        m_game->queue_event<event_type::on_effect_end>(this);
                    } else {
                        throw invalid_action();
                    }
                } else {
                    throw invalid_action();
                }
                break;
            case card_color_type::green:
                if (!has_card_equipped(card_it->name)) {
                    card_it->inactive = true;
                    deck_card removed = std::move(*card_it);
                    m_hand.erase(card_it);
                    equip_card(std::move(removed));
                    m_game->queue_event<event_type::on_equip>(this, this, args.card_id);
                    m_game->queue_event<event_type::on_effect_end>(this);
                    m_game->add_public_update<game_update_type::tap_card>(removed.id, true);
                } else {
                    throw invalid_action();
                }
                break;
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (m_num_drawn_cards != m_num_cards_to_draw) return;
            switch (card_it->color) {
            case card_color_type::blue:
                if (verify_card_targets(*card_it, false, args.targets)) {
                    do_play_card(args.card_id, false, args.targets);
                } else {
                    throw invalid_action();
                }
                break;
            case card_color_type::green:
                if (!card_it->inactive) {
                    if (verify_card_targets(*card_it, false, args.targets)) {
                        do_play_card(args.card_id, false, args.targets);
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            default:
                throw invalid_action();
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
            if (verify_card_targets(*card_it, true, args.targets) && can_respond(*card_it)) {
                do_play_card(args.card_id, true, args.targets);
            } else {
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            switch (card_it->color) {
            case card_color_type::brown:
                if (verify_card_targets(*card_it, true, args.targets) && can_respond(*card_it)) {
                    do_play_card(args.card_id, true, args.targets);
                } else {
                    throw invalid_action();
                }
                break;
            default:
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (!m_game->table_cards_disabled(id)) {
                switch (card_it->color) {
                case card_color_type::green:
                    if (!card_it->inactive) {
                        if (verify_card_targets(*card_it, true, args.targets) && can_respond(*card_it)) {
                            do_play_card(args.card_id, true, args.targets);
                        } else {
                            throw invalid_action();
                        }
                    }
                    break;
                case card_color_type::blue:
                    if (verify_card_targets(*card_it, true, args.targets) && can_respond(*card_it)) {
                        do_play_card(args.card_id, true, args.targets);
                    } else {
                        throw invalid_action();
                    }
                    break;
                default:
                    throw invalid_action();
                }
            }
        } else {
            throw game_error("server.respond_card: ID non trovato");
        }
    }

    void player::draw_from_deck() {
        if (std::ranges::find(m_characters, character_type::drawing_forced, &character::type) == m_characters.end()) {
            for (; m_num_drawn_cards<m_num_cards_to_draw; ++m_num_drawn_cards) {
                add_to_hand(m_game->draw_card());
            }
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
            obj.targets.emplace_back(value.target(), value.maxdistance());
        }
        m_virtual = std::make_pair(obj.virtual_id, std::move(c));

        m_game->add_private_update<game_update_type::virtual_card>(this, obj);
    }

    void player::start_of_turn() {
        m_game->m_playing = this;
        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        for (auto &c : m_characters) {
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

    void player::end_of_turn() {
        for (auto &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c.id, false);
            }
        }
        m_current_card_targets.clear();
        m_game->m_playing = m_game->get_next_player(this);
        m_game->queue_event<event_type::on_turn_end>(this);
        m_game->queue_event<event_type::delayed_action>([this]{
            if (m_game->num_alive() > 0) {
                m_game->m_playing->start_of_turn();
            }
        });
    }

    void player::discard_all() {
        for (deck_card &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c.id, false);
            }
            c.on_unequip(this);
            m_game->add_to_discards(std::move(c));
        }
        m_table.clear();
        for (deck_card &c : m_hand) {
            m_game->add_to_discards(std::move(c));
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