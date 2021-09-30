#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/responses.h"
#include "common/net_enums.h"

namespace banggame {
    constexpr auto is_weapon = [](const deck_card &c) {
        return !c.effects.empty() && c.effects.front().is<effect_weapon>();
    };

    void player::discard_weapon() {
        auto it = std::ranges::find_if(m_table, is_weapon);
        if (it != m_table.end()) {
            auto &moved = get_game()->add_to_discards(std::move(*it));
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
        get_game()->add_show_card(moved);
        get_game()->add_public_update<game_update_type::move_card>(moved.id, id, card_pile_type::player_table);
    }

    bool player::has_card_equipped(const std::string &name) const {
        return std::ranges::find(m_table, name, &deck_card::name) != m_table.end();
    }

    deck_card &player::get_hand_card(int card_id) {
        auto it = std::ranges::find(m_hand, card_id, &deck_card::id);
        if (it == m_hand.end()) throw game_error("ID non trovato");
        return *it;
    }

    deck_card &player::get_table_card(int card_id) {
        auto it = std::ranges::find(m_table, card_id, &deck_card::id);
        if (it == m_table.end()) throw game_error("ID non trovato");
        return *it;
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
                get_game()->add_public_update<game_update_type::tap_card>(it->id, false);
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
        return get_game()->add_to_discards(get_card_removed(card_id));
    }

    void player::steal_card(player *target, int card_id) {
        auto &moved = m_hand.emplace_back(target->get_card_removed(card_id));
        get_game()->add_show_card(moved, this);
        get_game()->add_public_update<game_update_type::move_card>(card_id, id, card_pile_type::player_hand);
    }

    void player::damage(player *source, int value) {
        m_hp -= value;
        get_game()->add_public_update<game_update_type::player_hp>(id, m_hp);
        if (m_hp <= 0) {
            get_game()->add_response<response_type::death>(source, this);
        }
    }

    void player::heal(int value) {
        m_hp = std::min(m_hp + value, m_max_hp);
        get_game()->add_public_update<game_update_type::player_hp>(id, m_hp);
    }

    void player::add_to_hand(deck_card &&target) {
        const auto &c = m_hand.emplace_back(std::move(target));
        get_game()->add_show_card(c, this);
        get_game()->add_public_update<game_update_type::move_card>(c.id, id, card_pile_type::player_hand);
    }

    bool player::verify_card_targets(const deck_card &c, const std::vector<play_card_target> &targets) {
        auto in_range = [this](int player_id, int distance) {
            return distance == 0 || get_game()->calc_distance(this, get_game()->get_player(player_id)) <= distance;
        };
        if (c.color == card_color_type::blue) {
            if (c.effects.empty() || targets.size() != 1) return false;
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
                return get_game()->get_player(tgts.front().player_id)->role() != player_role::sheriff
                    && in_range(tgts.front().player_id, c.effects.front()->maxdistance);
            case target_type::reachable:
                return tgts.front().player_id != id && in_range(tgts.front().player_id, m_weapon_range);
            case target_type::anyone:
                return true;
            default:
                return false;
            }
        }
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
                            p = get_game()->get_next_player(p);
                            if (p == this) break;
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    }
                    case target_type::others: {
                        std::vector<target_player_id> ids;
                        for (auto *p = this;;) {
                            p = get_game()->get_next_player(p);
                            if (p == this) break;
                            ids.emplace_back(p->id);
                        }
                        return std::ranges::equal(args, ids, {}, &target_player_id::player_id, &target_player_id::player_id);
                    }
                    case target_type::notsheriff:
                        return args.size() == 1 && get_game()->get_player(args.front().player_id)->role() != player_role::sheriff
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
                        return arg.from_hand && get_game()->get_player(arg.player_id)->is_hand_empty();
                    })) return false;
                    if (e->target == target_type::othercards) {
                        if (!std::ranges::all_of(get_game()->m_players | std::views::filter(&player::alive), [&](int player_id) {
                            bool found = std::ranges::find(args, player_id, &target_card_id::player_id) != args.end();
                            return (player_id == id) != found;
                        }, &player::id)) return false;
                        return std::ranges::all_of(args, [&](const target_card_id &arg) {
                            if (arg.player_id == id) return false;
                            if (arg.from_hand) return true;
                            auto &l = get_game()->get_player(arg.player_id)->m_table;
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
                            && get_hand_card(tgt.card_id).color == card_color_type::blue;
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

    void player::do_play_card(deck_card &c, const std::vector<play_card_target> &targets) {
        auto effect_it = targets.begin();
        for (auto &e : c.effects) {
            enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    e->on_play(this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        e->on_play(this, get_game()->get_player(target.player_id));
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_card>, const std::vector<target_card_id> &args) {
                    for (const auto &target : args) {
                        e->on_play(this, get_game()->get_player(target.player_id), target.card_id);
                    }
                }
            }, *effect_it++);
        }
    }

    void player::play_card(const play_card_args &args) {
        auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id);
        if (card_it == m_hand.end()) {
            card_it = std::ranges::find(m_table, args.card_id, &deck_card::id);
            if (card_it == m_table.end()) return;
            switch (card_it->color) {
            case card_color_type::green:
                if (!card_it->inactive) {
                    if (verify_card_targets(*card_it, args.targets)) {
                        deck_card removed = std::move(*card_it);
                        m_table.erase(card_it);
                        do_play_card(get_game()->add_to_discards(std::move(removed)), args.targets);
                    } else {
                        throw game_error("Target non validi");
                    }
                }
                break;
            default:
                throw game_error("Carta giocata non valida");
            }
        } else {
            switch (card_it->color) {
            case card_color_type::brown:
                if (verify_card_targets(*card_it, args.targets)) {
                    if (std::ranges::all_of(card_it->effects, [this](effect_holder &e) {
                        return e->can_play(this);
                    })) {
                        deck_card removed = std::move(*card_it);
                        m_hand.erase(card_it);
                        do_play_card(get_game()->add_to_discards(std::move(removed)), args.targets);
                    }
                } else {
                    throw game_error("Target non validi");
                }
                break;
            case card_color_type::blue:
                if (verify_card_targets(*card_it, args.targets)) {
                    auto *target = get_game()->get_player(
                        args.targets.front().get<play_card_target_type::target_player>().front().player_id);
                    if (!target->has_card_equipped(card_it->name)) {
                        deck_card removed = std::move(*card_it);
                        m_hand.erase(card_it);
                        target->equip_card(std::move(removed));
                    } else {
                        throw game_error("Carte duplicate");
                    }
                } else {
                    throw game_error("Target non validi");
                }
                break;
            case card_color_type::green:
                if (!has_card_equipped(card_it->name)) {
                    card_it->inactive = true;
                    deck_card removed = std::move(*card_it);
                    m_hand.erase(card_it);
                    equip_card(std::move(removed));
                    get_game()->add_public_update<game_update_type::tap_card>(removed.id, true);
                } else {
                    throw game_error("Carte duplicate");
                }
                break;
            }
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        auto *resp = get_game()->top_response().as<card_response>();
        if (!resp) return;

        auto card_it = std::ranges::find(m_hand, args.card_id, &deck_card::id);
        if (card_it == m_hand.end()) {
            card_it = std::ranges::find(m_table, args.card_id, &deck_card::id);
            if (card_it == m_table.end()) return;
            switch (card_it->color) {
            case card_color_type::green:
                if (!card_it->inactive) {
                    if (verify_card_targets(*card_it, args.targets)) {
                        if (resp->on_respond(&*card_it)) {
                            deck_card removed = std::move(*card_it);
                            m_table.erase(card_it);
                            do_play_card(get_game()->add_to_discards(std::move(removed)), args.targets);
                        }
                    } else {
                        throw game_error("Target non validi");
                    }
                }
                break;
            case card_color_type::blue:
                resp->on_respond(&*card_it);
                break;
            default:
                throw game_error("Carta giocata non valida");
            }
        } else if (card_it->color == card_color_type::brown) {
            if (verify_card_targets(*card_it, args.targets)) {
                if (resp->on_respond(&*card_it)) {
                    deck_card removed = std::move(*card_it);
                    m_hand.erase(card_it);
                    do_play_card(get_game()->add_to_discards(std::move(removed)), args.targets);
                }
            } else {
                throw game_error("Target non validi");
            }
        }
    }

    void player::start_of_turn() {
        get_game()->add_public_update<game_update_type::switch_turn>(id);

        m_bangs_played = 0;
        m_bangs_per_turn = 1;

        m_pending_predraw_checks = m_predraw_checks;
        if (m_pending_predraw_checks.empty()) {
            get_game()->queue_response<response_type::draw>(nullptr, this);
        } else {
            get_game()->queue_response<response_type::predraw>(nullptr, this);
        }
    }

    void player::next_predraw_check(int card_id) {
        if (alive()) {
            m_pending_predraw_checks.erase(std::ranges::find(m_pending_predraw_checks, card_id, &predraw_check_t::card_id));

            if (m_pending_predraw_checks.empty()) {
                get_game()->queue_response<response_type::draw>(nullptr, this);
            } else {
                get_game()->queue_response<response_type::predraw>(nullptr, this);
            }
        }
    }

    void player::end_of_turn() {
        for (auto &c : m_table) {
            if (c.inactive) {
                c.inactive = false;
                get_game()->add_public_update<game_update_type::tap_card>(c.id, false);
            }
        }
        m_pending_predraw_checks.clear();
    }

    void player::discard_all() {
        for (deck_card &c : m_table) {
            get_game()->add_to_discards(std::move(c));
        }
        for (deck_card &c : m_hand) {
            get_game()->add_to_discards(std::move(c));
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
                e->on_equip(this, 0);
            }
        }

        player_character_update obj;
        obj.player_id = id;
        obj.card_id = c.id;
        obj.max_hp = m_max_hp;
        obj.name = c.name;
        obj.image = c.image;
        for (const auto &e : c.effects) {
            obj.targets.emplace_back(e->target, e->maxdistance);
        }
        get_game()->add_public_update<game_update_type::player_character>(std::move(obj));

        if (role == player_role::sheriff) {
            get_game()->add_public_update<game_update_type::player_show_role>(id, m_role);
        } else {
            get_game()->add_private_update<game_update_type::player_show_role>(this, id, m_role);
        }
    }
}