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
        m_game->add_show_card(moved);
        m_game->add_public_update<game_update_type::move_card>(moved.id, id, card_pile_type::player_table);
    }

    bool player::has_card_equipped(const std::string &name) const {
        return std::ranges::find(m_table, name, &deck_card::name) != m_table.end();
    }

    deck_card &player::find_hand_card(int card_id) {
        if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            return *it;
        } else {
            throw game_error("server.find_hand_card: ID non trovato");
        }
    }

    deck_card &player::find_table_card(int card_id) {
        if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            return *it;
        } else {
            throw game_error("server.find_table_card: ID non trovato");
        }
    }

    deck_card &player::random_hand_card() {
        return m_hand[m_game->rng() % m_hand.size()];
    }

    card &player::find_any_card(int card_id) {
        if (auto it = std::ranges::find(m_characters, card_id, &character::id); it != m_characters.end()) {
            return *it;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            return *it;
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            return *it;
        } else {
            throw game_error("server.find_any_card: ID non trovato");
        }
    }

    deck_card player::get_card_removed(int card_id) {
        auto it = std::ranges::find(m_table, card_id, &deck_card::id);
        deck_card c;
        if (it == m_table.end()) {
            it = std::ranges::find(m_hand, card_id, &deck_card::id);
            if (it == m_hand.end()) throw game_error("server.get_card_removed: ID non trovato");
            c = std::move(*it);
            m_hand.erase(it);
        } else {
            if (it->inactive) {
                it->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(it->id, false);
            }
            c = std::move(*it);
            c.on_unequip(this);
            m_table.erase(it);
        }
        return c;
    }

    deck_card &player::discard_card(int card_id) {
        return m_game->add_to_discards(get_card_removed(card_id));
    }

    deck_card &player::discard_hand_card_response(int card_id) {
        auto it = std::ranges::find(m_hand, card_id, &card::id);
        auto &moved = m_game->add_to_discards(std::move(*it));
        m_hand.erase(it);
        if (m_game->m_playing != this) {
            m_game->queue_event<event_type::on_play_off_turn>(this, card_id);
        }
        return moved;
    }

    void player::steal_card(player *target, int card_id) {
        auto &moved = m_hand.emplace_back(target->get_card_removed(card_id));
        m_game->add_show_card(moved, this);
        m_game->add_public_update<game_update_type::move_card>(card_id, id, card_pile_type::player_hand);
    }

    void player::damage(player *source, int value) {
        m_hp -= value;
        m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        if (m_hp <= 0) {
            m_game->add_request<request_type::death>(source, this);
        }
        for (int i=0; i<value; ++i) {
            m_game->queue_event<event_type::on_hit>(source, this);
        }
    }

    void player::heal(int value) {
        m_hp = std::min(m_hp + value, m_max_hp);
        m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
    }

    void player::add_to_hand(deck_card &&target) {
        const auto &c = m_hand.emplace_back(std::move(target));
        m_game->add_show_card(c, this);
        m_game->add_public_update<game_update_type::move_card>(c.id, id, card_pile_type::player_hand);
    }

    bool player::verify_equip_target(const card &c, const std::vector<play_card_target> &targets) {
        if (targets.size() != 1) return false;
        if (targets.front().enum_index() != play_card_target_type::target_player) return false;
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (c.equips.empty()) return true;
        if (tgts.size() != 1) return false;
        player *target = m_game->get_player(tgts.front().player_id);
        auto in_range = [&](int distance) {
            return distance == 0 || m_game->calc_distance(this, target) <= distance;
        };
        return std::ranges::all_of(enums::enum_values_v<target_type>
            | std::views::filter([type = c.equips.front().target()](target_type value) {
                return bool(type & value);
            }), [&](target_type value) {
                switch (value) {
                case target_type::player: return true;
                case target_type::self: return target->id == id;
                case target_type::notself: return target->id != id;
                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                case target_type::reachable: return in_range(m_weapon_range);
                default: return false;
                }
            }) && in_range(c.equips.front().maxdistance());
    }

    bool player::verify_card_targets(const card &c, bool is_response, const std::vector<play_card_target> &targets) {
        auto &effects = is_response ? c.responses : c.effects;
        if (!std::ranges::all_of(effects, [this](const effect_holder &e) {
            return e.can_play(this);
        })) return false;

        if (effects.size() != targets.size()) return false;
        auto in_range = [&](player *target, int distance) {
            return distance == 0 || m_game->calc_distance(this, target) <= distance;
        };
        return std::ranges::all_of(effects, [&, it = targets.begin()] (const effect_holder &e) mutable {
            return enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    return e.target() == enums::flags_none<target_type>;
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    if (!bool(e.target() & target_type::player)) return false;
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
                                case target_type::player: return true;
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return in_range(target, m_weapon_range);
                                default: return false;
                                }
                            }) && in_range(target, e.maxdistance());
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
                                case target_type::card: return true;
                                case target_type::self: return target->id == id;
                                case target_type::notself: return target->id != id;
                                case target_type::notsheriff: return target->m_role != player_role::sheriff;
                                case target_type::reachable: return in_range(target, m_weapon_range);
                                case target_type::table: return !args.front().from_hand;
                                case target_type::hand: return args.front().from_hand;
                                case target_type::blue: return target->find_hand_card(args.front().card_id).color == card_color_type::blue;
                                case target_type::clubs: return target->find_hand_card(args.front().card_id).suit == card_suit_type::clubs;
                                case target_type::bang: {
                                    auto &c = target->find_hand_card(args.front().card_id);
                                    return !c.effects.empty() && c.effects.front().is(effect_type::bangcard);
                                }
                                case target_type::missed: {
                                    auto &c = target->find_hand_card(args.front().card_id);
                                    return !c.responses.empty() && c.responses.front().is(effect_type::missedcard);
                                }
                                case target_type::bangormissed: {
                                    auto &c = target->find_hand_card(args.front().card_id);
                                    if (!c.effects.empty()) return c.effects.front().is(effect_type::bangcard);
                                    else if (!c.responses.empty()) return c.responses.front().is(effect_type::missedcard);
                                }
                                default: return false;
                                }
                            }) && in_range(target, e.maxdistance());
                    }
                }
            }, *it++);
        });
    }

    void player::do_play_card(int card_id, bool is_response, const std::vector<play_card_target> &targets) {
        card *card_ptr = nullptr;
        bool is_character = false;
        if (auto it = std::ranges::find(m_characters, card_id, &character::id); it != m_characters.end()) {
            card_ptr = &*it;
            is_character = true;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            card_ptr = &discard_hand_card_response(card_id);
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            if (!m_game->table_cards_disabled(id)) {
                if (it->color == card_color_type::blue) {
                    card_ptr = &*it;
                } else {
                    deck_card removed = std::move(*it);
                    m_table.erase(it);
                    card_ptr = &m_game->add_to_discards(std::move(removed));
                }
            } else {
                throw invalid_action();
            }
        } else if (m_virtual && card_id == m_virtual->second.id) {
            discard_hand_card_response(m_virtual->first);
            card_ptr = &m_virtual->second;
        } else {
            throw game_error("server.do_play_card: ID non trovato");
        }

        auto check_immunity = [&](player *target) {
            if (m_virtual) return target->immune_to(m_virtual->second);
            if (is_character) return false;
            return target->immune_to(*static_cast<deck_card*>(card_ptr));
        };
        
        auto effect_it = targets.begin();
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        for (auto &e : effects) {
            enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    e.on_play(this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        e.on_play(this, p);
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        if (target.from_hand && p != this) {
                            e.on_play(this, p, p->random_hand_card().id);
                        } else {
                            e.on_play(this, p, target.card_id);
                        }
                    }
                }
            }, *effect_it++);
        }

        m_game->queue_event<event_type::on_effect_end>(this);
        m_virtual.reset();
    }

    void player::play_card(const play_card_args &args) {
        if (auto card_it = std::ranges::find(m_characters, args.card_id, &character::id); card_it != m_characters.end()) {
            switch (card_it->type) {
            case character_type::active:
                if (m_has_drawn && (card_it->max_usages == 0 || card_it->usages < card_it->max_usages)) {
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
                if (!m_has_drawn) {
                    if (verify_card_targets(*card_it, false, args.targets)) {
                        do_play_card(args.card_id, false, args.targets);
                        m_has_drawn = true;
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            default:
                break;
            }
        } else if (m_virtual && args.card_id == m_virtual->second.id) {
            if (verify_card_targets(m_virtual->second, false, args.targets)) {
                do_play_card(args.card_id, false, args.targets);
            } else {
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            if (!m_has_drawn) return;
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
                        m_game->queue_event<event_type::on_equip>(this, args.card_id);
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
                    m_game->queue_event<event_type::on_equip>(this, args.card_id);
                    m_game->queue_event<event_type::on_effect_end>(this);
                    m_game->add_public_update<game_update_type::tap_card>(removed.id, true);
                } else {
                    throw invalid_action();
                }
                break;
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            if (!m_has_drawn) return;
            switch (card_it->color) {
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
        if (m_game->m_requests.empty()) return;
        
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
            for (int i=0; i<m_num_drawn_cards; ++i) {
                add_to_hand(m_game->draw_card());
            }
            m_has_drawn = true;
        }
    }

    void player::play_virtual_card(deck_card &&c) {
        virtual_card_update obj;
        obj.card_id = c.id;
        obj.virtual_id = c.id = m_game->get_next_id();
        obj.color = c.color;
        obj.suit = c.suit;
        obj.value = c.value;
        for (const auto &value : c.effects) {
            obj.targets.emplace_back(value.target(), value.maxdistance());
        }
        m_virtual = std::make_pair(obj.card_id, std::move(c));

        m_game->add_private_update<game_update_type::virtual_card>(this, obj);
    }

    void player::start_of_turn() {
        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_has_drawn = false;
        for (auto &c : m_characters) {
            c.usages = 0;
        }
        
        m_game->add_public_update<game_update_type::switch_turn>(id);

        m_pending_predraw_checks = m_predraw_checks;
        if (m_pending_predraw_checks.empty()) {
            m_game->queue_event<event_type::on_turn_start>(this);
        } else {
            m_game->queue_request<request_type::predraw>(this, this);
        }
    }

    void player::next_predraw_check(int card_id) {
        if (alive()) {
            m_pending_predraw_checks.erase(std::ranges::find(m_pending_predraw_checks, card_id, &predraw_check_t::card_id));

            if (m_pending_predraw_checks.empty()) {
                m_game->queue_event<event_type::on_turn_start>(this);
            } else {
                m_game->queue_request<request_type::predraw>(this, this);
            }
        }
    }

    void player::end_of_turn() {
        for (auto &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c.id, false);
            }
        }
        m_pending_predraw_checks.clear();
        m_game->queue_event<event_type::on_turn_end>(this);
    }

    void player::discard_all() {
        for (deck_card &c : m_table) {
            m_game->add_to_discards(std::move(c));
        }
        for (deck_card &c : m_hand) {
            m_game->add_to_discards(std::move(c));
        }
    }

    void player::send_character_update(const character &c, int index) {
        player_character_update obj;
        obj.player_id = id;
        obj.card_id = c.id;
        obj.index = index;
        obj.max_hp = m_max_hp;
        obj.name = c.name;
        obj.image = c.image;
        obj.type = c.type;
        for (const auto &e : c.effects) {
            obj.targets.emplace_back(e.target(), e.maxdistance());
        }
        for (const auto &e : c.responses) {
            obj.response_targets.emplace_back(e.target(), e.maxdistance());
        }
        
        m_game->add_public_update<game_update_type::player_character>(std::move(obj));
    }

    void player::set_character_and_role(const character &c, player_role role) {
        m_characters.push_back(c);
        m_role = role;

        m_max_hp = c.max_hp;
        if (role == player_role::sheriff) {
            ++m_max_hp;
        }
        m_hp = m_max_hp;

        m_characters.front().on_equip(this);
        send_character_update(c, 0);

        if (role == player_role::sheriff) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role);
        }
    }
}