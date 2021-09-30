#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/responses.h"
#include "common/net_enums.h"

namespace banggame {
    static auto invalid_action() {
        return game_error{"Azione non valida"};
    }

    constexpr auto is_weapon = [](const deck_card &c) {
        return c.effects.front().is<effect_weapon>();
    };

    void player::discard_weapon() {
        auto it = std::ranges::find_if(m_table, is_weapon);
        if (it != m_table.end()) {
            auto &moved = m_game->add_to_discards(std::move(*it));
            for (auto &e : moved.effects) {
                e->on_unequip(this, moved.id);
            }
            m_table.erase(it);
        }
    }

    void player::equip_card(deck_card &&target) {
        for (auto &e : target.effects) {
            e->on_equip(this, target.id);
        }
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
            throw game_error("ID non trovato");
        }
    }

    deck_card &player::find_table_card(int card_id) {
        if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            return *it;
        } else {
            throw game_error("ID non trovato");
        }
    }

    deck_card &player::random_hand_card() {
        return m_hand[m_game->rng() % m_hand.size()];
    }

    card &player::find_any_card(int card_id) {
        if (card_id == m_character.id) {
            return m_character;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            return *it;
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            return *it;
        } else {
            throw game_error("ID non trovato");
        }
    }

    deck_card player::get_card_removed(int card_id) {
        auto it = std::ranges::find(m_table, card_id, &deck_card::id);
        deck_card c;
        if (it == m_table.end()) {
            it = std::ranges::find(m_hand, card_id, &deck_card::id);
            if (it == m_hand.end()) throw game_error("ID non trovato");
            c = std::move(*it);
            m_hand.erase(it);
        } else {
            if (it->inactive) {
                it->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(it->id, false);
            }
            c = std::move(*it);
            for (auto &e : c.effects) {
                e->on_unequip(this, card_id);
            }
            m_table.erase(it);
        }
        return c;
    }

    deck_card &player::discard_card(int card_id) {
        auto &moved = m_game->add_to_discards(get_card_removed(card_id));
        if (m_hand.empty()) {
            m_game->handle_game_event<event_type::on_empty_hand>(this);
        }
        return moved;
    }

    void player::steal_card(player *target, int card_id) {
        auto &moved = m_hand.emplace_back(target->get_card_removed(card_id));
        if (target->m_hand.empty()) {
            m_game->handle_game_event<event_type::on_empty_hand>(target);
        }
        m_game->add_show_card(moved, this);
        m_game->add_public_update<game_update_type::move_card>(card_id, id, card_pile_type::player_hand);
    }

    void player::damage(player *source, int value) {
        m_hp -= value;
        m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
        if (m_hp <= 0) {
            m_game->add_response<response_type::death>(source, this);
        }
        for (int i=0; i<value; ++i) {
            m_game->handle_game_event<event_type::on_hit>(source, this);
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
        auto in_range = [this](int player_id, int distance) {
            return distance == 0 || m_game->calc_distance(this, m_game->get_player(player_id)) <= distance;
        };
        if (targets.size() != 1) return false;
        if (targets.front().enum_index() != play_card_target_type::target_player) return false;
        const auto &tgts = targets.front().get<play_card_target_type::target_player>();
        if (tgts.size() != 1) return false;
        switch (c.effects.front()->target) {
        case target_type::none:
        case target_type::self:
            return tgts.front().player_id == id;
        case target_type::notself:
            return tgts.front().player_id != id && in_range(tgts.front().player_id, c.effects.front()->maxdistance);
        case target_type::notsheriff:
            return m_game->get_player(tgts.front().player_id)->m_role != player_role::sheriff
                && in_range(tgts.front().player_id, c.effects.front()->maxdistance);
        case target_type::reachable:
            return tgts.front().player_id != id && in_range(tgts.front().player_id, m_weapon_range);
        case target_type::anyone:
            return true;
        default:
            return false;
        }
    }

    bool player::verify_card_targets(const card &c, const std::vector<play_card_target> &targets) {
        if (!std::ranges::all_of(c.effects, [this](const effect_holder &e) {
            return e->can_play(this);
        })) return false;

        auto in_range = [this](int player_id, int distance) {
            return distance == 0 || m_game->calc_distance(this, m_game->get_player(player_id)) <= distance;
        };
        if (c.effects.size() != targets.size()) return false;
        return std::ranges::all_of(c.effects, [&, it = targets.begin()] (const effect_holder &e) mutable {
            return enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    return e->target == target_type::none || e->target == target_type::response;
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    switch (e->target) {
                    case target_type::self:
                        return args.size() == 1 && args.front().player_id == id;
                    case target_type::notself:
                        return args.size() == 1 && args.front().player_id != id
                            && in_range(args.front().player_id, e->maxdistance);
                    case target_type::everyone: {
                        std::vector<target_player_id> ids;
                        for (auto *p = this;;) {
                            ids.emplace_back(p->id);
                            p = m_game->get_next_player(p);
                            if (p == this) break;
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    }
                    case target_type::others: {
                        std::vector<target_player_id> ids;
                        for (auto *p = this;;) {
                            p = m_game->get_next_player(p);
                            if (p == this) break;
                            ids.emplace_back(p->id);
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    }
                    case target_type::notsheriff:
                        return args.size() == 1 && m_game->get_player(args.front().player_id)->m_role != player_role::sheriff
                            && in_range(args.front().player_id, c.effects.front()->maxdistance);
                    case target_type::reachable:
                        return args.size() == 1 && args.front().player_id != id && in_range(args.front().player_id, m_weapon_range);
                    case target_type::anyone:
                        return args.size() == 1;
                    default:
                        return false;
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    if (std::ranges::find(args, c.id, &target_card_id::card_id) != args.end()) return false;
                    if (std::ranges::any_of(args, [&](int player_id) {
                        return !in_range(player_id, e->maxdistance);
                    }, &target_card_id::player_id)) return false;
                    if (std::ranges::any_of(args, [&](const target_card_id &arg) {
                        return arg.from_hand && m_game->get_player(arg.player_id)->is_hand_empty();
                    })) return false;
                    if (e->target == target_type::othercards) {
                        if (!std::ranges::all_of(m_game->m_players | std::views::filter(&player::alive), [&](int player_id) {
                            bool found = std::ranges::find(args, player_id, &target_card_id::player_id) != args.end();
                            return (player_id == id) != found;
                        }, &player::id)) return false;
                        return std::ranges::all_of(args, [&](const target_card_id &arg) {
                            if (arg.player_id == id) return false;
                            if (arg.from_hand) return true;
                            auto &l = m_game->get_player(arg.player_id)->m_table;
                            return std::ranges::find(l, arg.card_id, &deck_card::id) != l.end();
                        });
                    } else if (args.size() == 1) {
                        const auto &tgt = args.front();
                        switch (e->target) {
                        case target_type::anycard: return true;
                        case target_type::table_card: return !tgt.from_hand;
                        case target_type::other_table_card: return !tgt.from_hand && tgt.player_id != id;
                        case target_type::other_hand_card: return tgt.from_hand && tgt.player_id != id;
                        case target_type::selfhand: return tgt.from_hand && tgt.player_id == id;
                        case target_type::selfhand_blue: return tgt.from_hand && tgt.player_id == id
                            && find_hand_card(tgt.card_id).color == card_color_type::blue;
                        default:
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            }, *it++);
        });
    }

    void player::do_play_card(int card_id, const std::vector<play_card_target> &targets) {
        card *card_ptr = nullptr;
        bool is_character = false;
        if (card_id == m_character.id) {
            card_ptr = &m_character;
            is_character = true;
        } else if (auto it = std::ranges::find(m_hand, card_id, &deck_card::id); it != m_hand.end()) {
            deck_card removed = std::move(*it);
            m_hand.erase(it);
            card_ptr = &m_game->add_to_discards(std::move(removed));
            if (m_game->m_playing != this) {
                m_game->handle_game_event<event_type::on_play_off_turn>(this, card_id);
            }
        } else if (auto it = std::ranges::find(m_table, card_id, &deck_card::id); it != m_table.end()) {
            if (it->color == card_color_type::blue) {
                card_ptr = &*it;
            } else {
                deck_card removed = std::move(*it);
                m_table.erase(it);
                card_ptr = &m_game->add_to_discards(std::move(removed));
            }
        } else {
            throw game_error("ID non trovato");
        }

        auto check_immunity = [&](player *target) {
            if (is_character) return false;
            return target->immune_to(*static_cast<deck_card*>(card_ptr));
        };
        
        auto effect_it = targets.begin();
        for (auto &e : card_ptr->effects) {
            enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    e->on_play(this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        e->on_play(this, p);
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    for (const auto &target : args) {
                        auto *p = m_game->get_player(target.player_id);
                        if (p != this && check_immunity(p)) continue;
                        if (target.from_hand && p != this) {
                            e->on_play(this, p, p->random_hand_card().id);
                        } else {
                            e->on_play(this, p, target.card_id);
                        }
                    }
                }
            }, *effect_it++);
        }

        if (m_hand.empty()) {
            m_game->handle_game_event<event_type::on_empty_hand>(this);
        }
    }

    void player::play_card(const play_card_args &args) {
        if (args.card_id == m_character.id) {
            switch (m_character.type) {
            case character_type::active:
                if (m_has_drawn && (m_character.usages == 0 || m_character_usages < m_character.usages)) {
                    if (verify_card_targets(m_character, args.targets)) {
                        do_play_card(args.card_id, args.targets);
                        ++m_character_usages;
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            case character_type::drawing:
            case character_type::drawing_forced:
                if (!m_has_drawn) {
                    if (verify_card_targets(m_character, args.targets)) {
                        do_play_card(args.card_id, args.targets);
                        m_has_drawn = true;
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            default:
                break;
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            if (!m_has_drawn) return;
            switch (card_it->color) {
            case card_color_type::brown:
                if (verify_card_targets(*card_it, args.targets)) {
                    do_play_card(args.card_id, args.targets);
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
                        m_game->handle_game_event<event_type::on_equip>(this, args.card_id);
                        if (m_hand.empty()) {
                            m_game->handle_game_event<event_type::on_empty_hand>(this);
                        }
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
                    m_game->handle_game_event<event_type::on_equip>(this, args.card_id);
                    m_game->add_public_update<game_update_type::tap_card>(removed.id, true);
                    if (m_hand.empty()) {
                        m_game->handle_game_event<event_type::on_empty_hand>(this);
                    }
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
                    if (verify_card_targets(*card_it, args.targets)) {
                        do_play_card(args.card_id, args.targets);
                    } else {
                        throw invalid_action();
                    }
                }
                break;
            default:
                throw invalid_action();
            }
        } else {
            throw game_error("ID carta non trovato");
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        auto *resp = m_game->top_response().as<card_response>();
        if (!resp) return;

        if (m_character.id == args.card_id) {
            if (verify_card_targets(m_character, args.targets)) {
                resp->on_respond(args);
            } else {
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id); card_it != m_hand.end()) {
            switch (card_it->color) {
            case card_color_type::brown:
                if (verify_card_targets(*card_it, args.targets)) {
                    resp->on_respond(args);
                } else {
                    throw invalid_action();
                }
                break;
            default:
                throw invalid_action();
            }
        } else if (auto card_it = std::ranges::find(m_table, args.card_id, &deck_card::id); card_it != m_table.end()) {
            switch (card_it->color) {
            case card_color_type::green:
                if (!card_it->inactive) {
                    if (verify_card_targets(*card_it, args.targets)) {
                        resp->on_respond(args);
                    } else {
                        throw game_error("Target non validi");
                    }
                }
                break;
            case card_color_type::blue:
                resp->on_respond(args);
                break;
            default:
                throw invalid_action();
            }
        } else {
            throw game_error("ID carta non trovato");
        }
    }

    void player::draw_from_deck() {
        if (m_character.type != character_type::drawing_forced) {
            for (int i=0; i<m_num_drawn_cards; ++i) {
                add_to_hand(m_game->draw_card());
            }
            m_has_drawn = true;
        }
    }

    void player::start_of_turn() {
        m_game->add_public_update<game_update_type::switch_turn>(id);

        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_character_usages = 0;
        m_has_drawn = false;

        m_pending_predraw_checks = m_predraw_checks;
        if (!m_pending_predraw_checks.empty()) {
            m_game->queue_response<response_type::predraw>(nullptr, this);
        }
    }

    void player::next_predraw_check(int card_id) {
        if (alive()) {
            m_pending_predraw_checks.erase(std::ranges::find(m_pending_predraw_checks, card_id, &predraw_check_t::card_id));

            if (!m_pending_predraw_checks.empty()) {
                m_game->queue_response<response_type::predraw>(nullptr, this);
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
    }

    void player::discard_all() {
        for (deck_card &c : m_table) {
            m_game->add_to_discards(std::move(c));
        }
        for (deck_card &c : m_hand) {
            m_game->add_to_discards(std::move(c));
        }
    }

    void player::set_character_and_role(const character &c, player_role role) {
        m_character = c;
        m_role = role;

        m_max_hp = c.max_hp;
        if (role == player_role::sheriff) {
            ++m_max_hp;
        }
        m_hp = m_max_hp;

        if (m_character.type == character_type::none) {
            for (auto &e : m_character.effects) {
                e->on_equip(this, m_character.id);
            }
        }

        player_character_update obj;
        obj.player_id = id;
        obj.card_id = c.id;
        obj.max_hp = m_max_hp;
        obj.name = c.name;
        obj.image = c.image;
        obj.type = c.type;
        for (const auto &e : c.effects) {
            obj.targets.emplace_back(e->target, e->maxdistance);
        }
        m_game->add_public_update<game_update_type::player_character>(std::move(obj));

        if (role == player_role::sheriff) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role);
        }
    }
}