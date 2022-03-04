#include "target_finder.h"

#include "game.h"
#include "../manager.h"
#include "../os_api.h"

#include "utils/utils.h"

#include <cassert>

using namespace banggame;
using namespace enums::flag_operators;

template<game_action_type T, typename ... Ts>
void target_finder::add_action(Ts && ... args) {
    m_game->parent->add_message<client_message_type::game_action>(enums::enum_constant<T>{}, std::forward<Ts>(args) ...);
}

void target_finder::set_border_colors() {
    for (auto [pile, player, card] : m_picking_highlights) {
        switch (pile) {
        case card_pile_type::player_hand:
        case card_pile_type::player_table:
        case card_pile_type::player_character:
        case card_pile_type::selection:
            card->border_color = options::target_finder_can_pick_rgba;
            break;
        case card_pile_type::main_deck:
            m_game->m_main_deck.border_color = options::target_finder_can_pick_rgba;
            break;
        case card_pile_type::discard_pile:
            m_game->m_discard_pile.border_color = options::target_finder_can_pick_rgba;
            break;
        }
    }
    for (auto *card : m_response_highlights) {
        card->border_color = options::target_finder_can_respond_rgba;
    }
    for (auto *card : m_modifiers) {
        card->border_color = options::target_finder_current_card_rgba;
    }

    if (m_playing_card) {
        m_playing_card->border_color = options::target_finder_current_card_rgba;
    }

    for (auto &[value, is_auto] : m_targets) {
        if (!is_auto) {
            std::visit(util::overloaded{
                [](target_none) {},
                [](target_other_players) {},
                [](target_player p) {
                    p.player->border_color = options::target_finder_target_rgba;
                },
                [&](target_card c) {
                    if (std::ranges::find(m_selected_cubes, c.card, &cube_widget::owner) == m_selected_cubes.end()) {
                        c.card->border_color = options::target_finder_target_rgba;
                    }
                },
                [](const target_cards &cs) {
                    for (target_card c : cs) {
                        c.card->border_color = options::target_finder_target_rgba;
                    }
                }
            }, value);
        }
    }
    for (auto *cube : m_selected_cubes) {
        cube->border_color = options::target_finder_target_rgba;
    }
}

bool target_finder::is_card_clickable() const {
    return m_game->m_pending_updates.empty() && m_game->m_animations.empty() && !waiting_confirm();
}

bool target_finder::can_respond_with(card_view *card) const {
    return std::ranges::find(m_response_highlights, card) != m_response_highlights.end();
}

bool target_finder::can_pick(card_pile_type pile, player_view *player, card_view *card) const {
    return std::ranges::find(m_picking_highlights, std::tuple{pile, player, card}) != m_picking_highlights.end();
}

bool target_finder::can_play_in_turn(player_view *player, card_view *card) const {
    return m_game->m_request_origin_id == 0
        && m_game->m_request_target_id == 0
        && m_game->m_playing_id == m_game->m_player_own_id
        && (!player || player->id == m_game->m_player_own_id);
}

void target_finder::set_response_highlights(const request_status_args &args) {
    clear_status();

    for (int id : args.respond_ids) {
        m_response_highlights.push_back(m_game->find_card(id));
    }

    for (const picking_args &args : args.pick_ids) {
        m_picking_highlights.emplace_back(args.pile,
            args.player_id ? m_game->find_player(args.player_id) : nullptr,
            args.card_id ? m_game->find_card(args.card_id) : nullptr);
    }
}

void target_finder::clear_status() {
    for (card_view *card : m_response_highlights) {
        card->border_color = 0;
    }
    for (auto &[pile, player, card] : m_picking_highlights) {
        switch (pile) {
        case card_pile_type::player_hand:
        case card_pile_type::player_table:
        case card_pile_type::player_character:
        case card_pile_type::selection:
            card->border_color = 0;
            break;
        case card_pile_type::main_deck:
            m_game->m_main_deck.border_color = 0;
            break;
        case card_pile_type::discard_pile:
            m_game->m_discard_pile.border_color = 0;
            break;
        }
    }

    m_response_highlights.clear();
    m_picking_highlights.clear();
}

void target_finder::clear_targets() {
    if (m_playing_card) {
        m_playing_card->border_color = 0;
    }
    for (card_view *card : m_modifiers) {
        card->border_color = 0;
    }
    for (auto &[value, is_auto] : m_targets) {
        if (!is_auto) {
            std::visit(util::overloaded{
                [](target_none) {},
                [](target_other_players) {},
                [this](target_player p) {
                    p.player->border_color = m_game->m_playing_id == p.player->id ? options::turn_indicator_rgba : 0;
                },
                [](target_card c) {
                    c.card->border_color = 0;
                },
                [](const target_cards &cs) {
                    for (target_card c : cs) {
                        c.card->border_color = 0;
                    }
                }
            }, value);
        }
    }
    for (cube_widget *cube : m_selected_cubes) {
        cube->border_color = 0;
    }
    m_game->m_shop_choice.clear();
    static_cast<target_status &>(*this) = {};
    if (m_forced_card) {
        set_forced_card(m_forced_card);
    }
}

bool target_finder::can_confirm() const {
    if (m_playing_card && !m_equipping) {
        const int neffects = get_current_card_effects().size();
        const int noptionals = get_optional_effects().size();
        return noptionals != 0
            && m_targets.size() >= neffects
            && ((m_targets.size() - neffects) % noptionals == 0);
    }
    return false;
}

void target_finder::on_click_confirm() {
    if (can_confirm()) send_play_card();
}

void target_finder::set_forced_card(card_view *card) {
    m_forced_card = card;
    if (!m_forced_card) {
        return;
    }

    if (card->pile == &m_game->find_player(m_game->m_player_own_id)->table || card->color == card_color_type::brown) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (verify_modifier(card)) {
            m_playing_card = card;
            handle_auto_targets();
        }
    } else {
        if (card->self_equippable()) {
            m_playing_card = card;
            m_targets.emplace_back(target_player{m_game->find_player(m_game->m_playing_id)}, true);
            send_play_card();
        } else {
            m_playing_card = card;
            m_equipping = true;
        }
    }
}

void target_finder::send_pick_card(card_pile_type pile, player_view *player, card_view *card) {
    add_action<game_action_type::pick_card>(pile, player ? player->id : 0, card ? card->id : 0);
    m_waiting_confirm = true;
}

void target_finder::on_click_discard_pile() {
    if (can_pick(card_pile_type::discard_pile, nullptr, nullptr)) {
        send_pick_card(card_pile_type::discard_pile);
    }
}

void target_finder::on_click_main_deck() {
    if (can_pick(card_pile_type::main_deck, nullptr, nullptr)) {
        send_pick_card(card_pile_type::main_deck);
    }
}

void target_finder::on_click_selection_card(card_view *card) {
    if (can_pick(card_pile_type::selection, nullptr, card)) {
        send_pick_card(card_pile_type::selection, nullptr, card);
    }
}

void target_finder::on_click_shop_card(card_view *card) {
    if (!m_playing_card
        && m_game->m_playing_id == m_game->m_player_own_id
        && m_game->m_request_origin_id == 0
        && m_game->m_request_target_id == 0)
    {
        int cost = card->buy_cost();
        if (std::ranges::find(m_modifiers, card_modifier_type::discount, &card_view::modifier) != m_modifiers.end()) {
            --cost;
        }
        if (m_game->find_player(m_game->m_player_own_id)->gold >= cost) {
            if (card->color == card_color_type::black) {
                if (verify_modifier(card)) {
                    if (card->self_equippable()) {
                        m_playing_card = card;
                        m_targets.emplace_back(target_player{m_game->find_player(m_game->m_playing_id)}, true);
                        send_play_card();
                    } else {
                        m_playing_card = card;
                        m_equipping = true;
                    }
                }
            } else if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    }
}

void target_finder::on_click_table_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card) && !card->inactive) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(card_pile_type::player_table, player, card)) {
            send_pick_card(card_pile_type::player_table, player, card);
        } else if (can_play_in_turn(player, card) && !card->inactive) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(target_card{player, card});
    }
}

void target_finder::on_click_hand_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(card_pile_type::player_hand, player, card)) {
            send_pick_card(card_pile_type::player_hand, player, card);
        } else if (can_play_in_turn(player, card)) {
            if (card->color == card_color_type::brown) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else if (verify_modifier(card)) {
                    m_playing_card = card;
                    handle_auto_targets();
                }
            } else if (m_modifiers.empty()) {
                if (card->self_equippable()) {
                    m_targets.emplace_back(target_player{player}, true);
                    m_playing_card = card;
                    send_play_card();
                } else {
                    m_playing_card = card;
                    m_equipping = true;
                }
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(target_card{player, card});
    }
}

void target_finder::on_click_character(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(card_pile_type::player_character, player, card)) {
            send_pick_card(card_pile_type::player_character, player, card);
        } else if (can_play_in_turn(player, card)) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (!card->effects.empty()) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_character_target(target_card{player, card});
    }
}

void target_finder::on_click_scenario_card(card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_play_in_turn(nullptr, card)) {
            m_playing_card = card;
            handle_auto_targets();
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    auto verify_target = [&](const auto &args) {
        if constexpr (requires { args.target; }) {
            if (args.target != play_card_target_type::player) {
                return false;
            }
        }
        if (verify_player_target(args.player_filter, player))  {
            return true;
        } else {
            os_api::play_bell();
            return false;
        }
    };

    if (m_playing_card) {
        if (m_equipping) {
            if (verify_target(m_playing_card->equips[m_targets.size()])) {
                m_targets.emplace_back(target_player{player});
                send_play_card();
                return true;
            }
        } else if (std::ranges::find(m_targets, target_variant_base{target_player{player}}, &target_variant::value) == m_targets.end()
            && verify_target(get_effect_holder(get_target_index())))
        {
            m_targets.emplace_back(target_player{player});
            handle_auto_targets();
            return true;
        }
    }
    return false;
}

void target_finder::add_modifier(card_view *card) {
    if (std::ranges::find(m_modifiers, card) == m_modifiers.end()) {
        switch (card->modifier) {
        case card_modifier_type::bangcard:
            if (std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::bangcard;
            })) {
                m_modifiers.push_back(card);
            }
            break;
        case card_modifier_type::leevankliff:
            if (m_modifiers.empty() && m_last_played_card) {
                m_modifiers.push_back(card);
            }
            break;
        case card_modifier_type::discount:
        case card_modifier_type::shopchoice:
            if (std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::discount
                    || c->modifier == card_modifier_type::shopchoice;
            })) {
                if (card->modifier == card_modifier_type::shopchoice) {
                    for (card_view *c : m_game->m_hidden_deck) {
                        if (!c->effects.empty() && c->effects.front().is(card->effects.front().type)) {
                            m_game->m_shop_choice.push_back(c);
                        }
                    }
                    for (card_view *c : m_game->m_shop_choice) {
                        c->set_pos(m_game->m_shop_choice.get_position_of(c));
                    }
                }
                m_modifiers.push_back(card);
            }
            break;
        default:
            break;
        }
    }
}

bool target_finder::is_bangcard(card_view *card) {
    return m_game->has_player_flags(player_flags::treat_any_as_bang)
        || (m_game->has_player_flags(player_flags::treat_missed_as_bang)
            && !card->responses.empty() && card->responses.front().is(effect_type::missedcard))
        || std::ranges::find(card->effects, effect_type::bangcard, &effect_holder::type) != card->effects.end();
}

bool target_finder::verify_modifier(card_view *card) {
    return std::ranges::all_of(m_modifiers, [&](card_view *c) {
        switch (c->modifier) {
        case card_modifier_type::bangcard:
            return is_bangcard(card);
        case card_modifier_type::leevankliff:
            return is_bangcard(card) && m_last_played_card;
        case card_modifier_type::discount:
            return card->expansion == card_expansion_type::goldrush;
        case card_modifier_type::shopchoice:
            return std::ranges::find(m_game->m_shop_choice, card) != m_game->m_shop_choice.end();
        default:
            return false;
        }
    });
}

const std::vector<effect_holder> &target_finder::get_current_card_effects() const {
    assert(!m_equipping);

    card_view *card = nullptr;
    if (m_last_played_card && !m_modifiers.empty() && m_modifiers.front()->modifier == card_modifier_type::leevankliff) {
        card = m_last_played_card;
    } else {
        card = m_playing_card;
    }
    assert(card != nullptr);

    if (m_response) {
        return card->responses;
    } else {
        return card->effects;
    }
}

const std::vector<effect_holder> &target_finder::get_optional_effects() const {
    if (m_last_played_card && !m_modifiers.empty() && m_modifiers.front()->modifier == card_modifier_type::leevankliff) {
        return m_last_played_card->optionals;
    } else {
        return m_playing_card->optionals;
    }
}

const effect_holder &target_finder::get_effect_holder(int index) {
    auto &effects = get_current_card_effects();
    if (index < effects.size()) {
        return effects[index];
    }

    auto &optionals = get_optional_effects();
    return optionals[(index - effects.size()) % optionals.size()];
}

int target_finder::num_targets_for(const effect_holder &data) {
    if (data.target == play_card_target_type::cards_other_players) {
        return std::ranges::count_if(m_game->m_players | std::views::values, [&](const player_view &p) {
            if (p.dead || p.id == m_game->m_player_own_id) return false;
            if (p.table.empty() && p.hand.empty()) return false;
            return true;
        });
    } else {
        return 1;
    }
};

int target_finder::get_target_index() {
    if (m_targets.empty()) return 0;
    int index = m_targets.size() - 1;
    size_t size = std::visit(util::overloaded{
        [](const auto &) -> size_t { return 1; },
        []<typename T>(const std::vector<T> &value) { return value.size(); }
    }, m_targets[index].value);
    index += size >= num_targets_for(get_effect_holder(index));
    return index;
}

int target_finder::calc_distance(player_view *from, player_view *to) {
    auto get_next_player = [&](player_view *p) {
        auto it = std::ranges::find(m_game->m_players, p, [](auto &pair) { return &pair.second; });
        do {
            if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
        } while(it->second.dead);
        return &it->second;
    };

    if (from == to) return 0;
    if (from->has_player_flags(player_flags::disable_player_distances)) return to->m_distance_mod;
    if (from->has_player_flags(player_flags::see_everyone_range_1)) return 1;
    int d1=0, d2=0;
    for (player_view *counter = from; counter != to; counter = get_next_player(counter), ++d1);
    for (player_view *counter = to; counter != from; counter = get_next_player(counter), ++d2);
    return std::min(d1, d2) + to->m_distance_mod;
}

void target_finder::handle_auto_targets() {
    using namespace enums::flag_operators;

    auto *self_player = m_game->find_player(m_game->m_player_own_id);
    auto do_handle_target = [&](const effect_holder &data) {
        switch (data.target) {
        case play_card_target_type::none:
            m_targets.emplace_back(target_none{}, true);
            return true;
        case play_card_target_type::player:
            if (data.player_filter == target_player_filter::self) {
                m_targets.emplace_back(target_player{self_player}, true);
                return true;
            }
            break;
        case play_card_target_type::other_players:
            m_targets.emplace_back(target_other_players{}, true);
            return true;
        }
        return false;
    };

    auto &effects = get_current_card_effects();
    auto &optionals = get_optional_effects();
    bool repeatable = !optionals.empty() && optionals.back().is(effect_type::repeatable);
    if (effects.empty()) {
        clear_targets();
    } else {
        auto effect_it = effects.begin() + m_targets.size();
        auto target_end = effects.end();
        if (effect_it >= target_end && !optionals.empty()) {
            int diff = m_targets.size() - effects.size();
            effect_it = optionals.begin() + (repeatable ? diff % optionals.size() : diff);
            target_end = optionals.end();
        }
        for(;;++effect_it) {
            if (effect_it == target_end) {
                if (optionals.empty() || (target_end == optionals.end()
                    && (!repeatable || (optionals.back().effect_value > 0
                        && m_targets.size() - effects.size() == optionals.size() * optionals.back().effect_value))))
                {
                    send_play_card();
                    break;
                }
                effect_it = optionals.begin();
                target_end = optionals.end();
            }
            if (!do_handle_target(*effect_it)) break;
        }
    }
}

bool target_finder::verify_player_target(target_player_filter filter, player_view *target_player) {
    player_view *own_player = m_game->find_player(m_game->m_player_own_id);

    if (target_player->dead != bool(filter & target_player_filter::dead)) return false;

    return std::ranges::all_of(enums::enum_flag_values(filter), [&](target_player_filter value) {
        switch (value) {
        case target_player_filter::self: return target_player->id == m_game->m_player_own_id;
        case target_player_filter::notself: return target_player->id != m_game->m_player_own_id;
        case target_player_filter::notsheriff: return target_player->m_role.role != player_role::sheriff;
        case target_player_filter::reachable:
            return own_player->m_weapon_range > 0
                && calc_distance(own_player, target_player) <= own_player->m_weapon_range + own_player->m_range_mod;
        case target_player_filter::range_1:
            return calc_distance(own_player, target_player) <= 1 + own_player->m_range_mod;
        case target_player_filter::range_2:
            return calc_distance(own_player, target_player) <= 2 + own_player->m_range_mod;
        case target_player_filter::dead:
            return true;
        default:
            return false;
        }
    });
}

bool target_finder::verify_card_target(const effect_holder &args, target_card target) {
    if (!verify_player_target(args.player_filter, target.player)) return false;
    if ((target.card->color == card_color_type::black) != bool(args.card_filter & target_card_filter::black)) return false;

    return std::ranges::all_of(enums::enum_flag_values(args.card_filter), [&](target_card_filter value) {
        switch (value) {
        case target_card_filter::black:
        case target_card_filter::can_repeat: return true;
        case target_card_filter::table: return target.card->pile == &target.player->table;
        case target_card_filter::hand: return target.card->pile == &target.player->hand;
        case target_card_filter::blue: return target.card->color == card_color_type::blue;
        case target_card_filter::clubs: return target.card->suit == card_suit_type::clubs;
        case target_card_filter::bang: return is_bangcard(target.card);
        case target_card_filter::missed: return !target.card->responses.empty() && target.card->responses.front().is(effect_type::missedcard);
        case target_card_filter::beer: return !target.card->effects.empty() && target.card->effects.front().is(effect_type::beer);
        case target_card_filter::bronco: return !target.card->equips.empty() && target.card->equips.back().is(equip_type::bronco);
        case target_card_filter::cube_slot:
        case target_card_filter::cube_slot_card:
            return target.card->color == card_color_type::orange
                || target.card == target.player->m_characters.front();
        default: return false;
        }
    });
}

void target_finder::add_card_target(target_card target) {
    if (m_equipping) return;

    int index = get_target_index();
    auto cur_target = get_effect_holder(index);

    if (cur_target.target == play_card_target_type::card || cur_target.target == play_card_target_type::cards_other_players) {
        if (!verify_card_target(cur_target, target)) {
            os_api::play_bell();
            return;
        }
    } else {
        return;
    }
    
    if (bool(cur_target.card_filter & target_card_filter::cube_slot)) {
        int ncubes = std::ranges::count(m_selected_cubes, target.card, &cube_widget::owner);
        if (ncubes < target.card->cubes.size()) {
            m_targets.emplace_back(target);
            m_selected_cubes.push_back(*(target.card->cubes.rbegin() + ncubes));
            handle_auto_targets();
        }
    } else if (target.card != m_playing_card && std::ranges::find(m_modifiers, target.card) == m_modifiers.end()) {
        if (cur_target.target == play_card_target_type::cards_other_players) {
            if (index >= m_targets.size()) {
                m_targets.emplace_back(target_cards{});
            }
            auto &vec = std::get<target_cards>(m_targets.back().value);
            if (target.player->id != m_game->m_player_own_id
                && std::ranges::find(vec, target.player, &target_card::player) == vec.end())
            {
                vec.push_back(target);
                if (vec.size() == num_targets_for(cur_target)) {
                    handle_auto_targets();
                }
            }
        } else if (bool(cur_target.card_filter & target_card_filter::can_repeat)
            || std::ranges::find(m_targets, target_variant_base{target}, &target_variant::value) == m_targets.end())
        {
            m_targets.emplace_back(target);
            handle_auto_targets();
        }
    }
}

void target_finder::add_character_target(target_card target) {
    if (m_equipping) return;

    const auto &effect = get_effect_holder(get_target_index());
    if (!bool(effect.card_filter & (target_card_filter::cube_slot | target_card_filter::cube_slot_card))) return;
    
    if (std::ranges::find(target.player->m_characters, target.card) != target.player->m_characters.end()) {
        target.card = target.player->m_characters.front();
    } else {
        return;
    }

    if (!verify_card_target(effect, target)) {
        os_api::play_bell();
        return;
    }

    if(bool(effect.card_filter & target_card_filter::cube_slot_card)) {
        m_targets.emplace_back(target);
        handle_auto_targets();
    } else {
        int ncubes = std::ranges::count(m_selected_cubes, target.card, &cube_widget::owner);
        if (ncubes < target.card->cubes.size()) {
            m_targets.emplace_back(target);
            m_selected_cubes.push_back(*(target.card->cubes.rbegin() + ncubes));
            handle_auto_targets();
        }
    }
}

void target_finder::send_play_card() {
    play_card_args ret;
    ret.card_id = m_playing_card ? m_playing_card->id : 0;
    for (card_view *card : m_modifiers) {
        ret.modifier_ids.push_back(card->id);
    }

    for (const auto &target : m_targets) {
        std::visit(util::overloaded{
            [&](target_none) {
                ret.targets.emplace_back(enums::enum_constant<play_card_target_type::none>());
            },
            [&](target_player p) {
                ret.targets.emplace_back(enums::enum_constant<play_card_target_type::player>(), p.player->id);
            },
            [&](target_other_players) {
                ret.targets.emplace_back(enums::enum_constant<play_card_target_type::other_players>());
            },
            [&](target_card c) {
                ret.targets.emplace_back(enums::enum_constant<play_card_target_type::card>(), c.card->id);
            },
            [&](const target_cards &cs) {
                std::vector<int> ids;
                for (auto [player, card] : cs) {
                    ids.push_back(card->id);
                }
                ret.targets.emplace_back(enums::enum_constant<play_card_target_type::cards_other_players>(), std::move(ids));
            }
        }, target.value);
    }
    if (m_response) {
        add_action<game_action_type::respond_card>(std::move(ret));
    } else {
        add_action<game_action_type::play_card>(std::move(ret));
    }

    m_waiting_confirm = true;
}

void target_finder::confirm_play() {
    m_waiting_confirm = false;
    m_forced_card = nullptr;
    clear_targets();
}