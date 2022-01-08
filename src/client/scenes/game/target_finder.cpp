#include "target_finder.h"

#include "game.h"
#include "../../manager.h"

#include <cassert>

using namespace banggame;
using namespace enums::flag_operators;

template<game_action_type T, typename ... Ts>
void target_finder::add_action(Ts && ... args) {
    m_game->parent->add_message<client_message_type::game_action>(enums::enum_constant<T>{}, std::forward<Ts>(args) ...);
}

static bool is_valid_picking_pile(request_type type, card_pile_type pile) {
    return enums::visit_enum([&](auto enum_const) {
        constexpr request_type E = decltype(enum_const)::value;
        if constexpr (picking_request<E>) {
            return enums::enum_type_t<E>::valid_pile(pile);
        } else {
            return false;
        }
    }, type);
}

void target_finder::set_response_highlights(const std::vector<int> &card_ids) {
    m_response_highlights.clear();
    for (int id : card_ids) {
        m_response_highlights.push_back(m_game->find_card(id));
    }
}

void target_finder::render(sdl::renderer &renderer) {
    renderer.set_draw_color(sdl::rgba(sizes::target_finder_can_respond_rgba));
    for (auto *card : m_response_highlights) {
        renderer.draw_rect(card->get_rect());
    }
    renderer.set_draw_color(sdl::rgba(sizes::target_finder_current_card_rgba));
    for (auto *card : m_modifiers) {
        renderer.draw_rect(card->get_rect());
    }
    if (m_playing_card) {
        renderer.draw_rect(m_playing_card->get_rect());
    }
    renderer.set_draw_color(sdl::rgba(sizes::target_finder_target_rgba));
    for (auto &l : m_targets) {
        for (auto [player, card] : l) {
            if (card) {
                if (std::ranges::find(m_selected_cubes, card, &cube_widget::owner) == m_selected_cubes.end()) {
                    renderer.draw_rect(card->get_rect());
                }
            } else if (player && player->id != m_game->m_player_own_id) {
                renderer.draw_rect(player->m_bounding_rect);
            }
        }
    }
    for (auto *cube : m_selected_cubes) {
        renderer.draw_rect(cube->get_rect());
    }
}

void target_finder::on_click_pass_turn() {
    add_action<game_action_type::pass_turn>();
}

void target_finder::on_click_resolve() {
    if (!m_playing_card && m_modifiers.empty()) {
        add_action<game_action_type::resolve>();
    } else if (m_playing_card && !bool(m_flags & play_card_flags::equipping) && !get_optional_targets().empty()) {
        if ((m_targets.size() - get_current_card_targets().size()) % get_optional_targets().size() == 0) {
            send_play_card();
        }
    }
}

void target_finder::on_click_main_deck() {
    if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id
        && is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::main_deck)) {
        add_action<game_action_type::pick_card>(card_pile_type::main_deck);
    } else if (m_game->m_playing_id == m_game->m_player_own_id && !m_game->has_player_flags(player_flags::has_drawn)) {
        auto *player = m_game->find_player(m_game->m_playing_id);
        add_action<game_action_type::draw_from_deck>();
    }
}

void target_finder::on_click_selection_card(card_view *card) {
    if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id
        && is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::selection)) {
        add_action<game_action_type::pick_card>(card_pile_type::selection, 0, card->id);
    }
}

void target_finder::on_click_shop_card(card_view *card) {
    m_flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (!m_playing_card) {
        if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id) {
            if (card->color == card_color_type::none) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            }
        } else if (m_game->m_playing_id == m_game->m_player_own_id && m_game->has_player_flags(player_flags::has_drawn)) {
            if (!verify_modifier(card)) return;
            if (card->color == card_color_type::black) {
                if (card->equip_targets.empty()
                    || card->equip_targets.front().target == enums::flags_none<target_type>
                    || bool(card->equip_targets.front().target & target_type::self)) {
                    m_playing_card = card;
                    m_targets.emplace_back(std::vector{target_pair{m_game->find_player(m_game->m_playing_id)}});
                    send_play_card();
                } else {
                    m_playing_card = card;
                    m_flags |= play_card_flags::equipping;
                }
            } else {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    }
}

void target_finder::on_click_table_card(player_view *player, card_view *card) {
    m_flags &= ~play_card_flags::sell_beer;

    if (bool(m_flags & play_card_flags::discard_black)) {
        m_targets.emplace_back(std::vector{target_pair{player, card}});
        send_play_card();
    } else if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (std::ranges::find(m_response_highlights, card) != m_response_highlights.end() && !card->inactive) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            } else if (is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::player_table)) {
                add_action<game_action_type::pick_card>(card_pile_type::player_table, player->id, card->id);
            }
        } else {
            add_card_target(target_pair{player, card});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (player->id == m_game->m_player_own_id && !card->inactive && m_game->has_player_flags(player_flags::has_drawn)) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else if (verify_modifier(card)) {
                    m_playing_card = card;
                    handle_auto_targets();
                }
            }
        } else {
            add_card_target(target_pair{player, card});
        }
    }
}

void target_finder::on_click_hand_card(player_view *player, card_view *card) {
    m_flags &= ~play_card_flags::discard_black;

    if (bool(m_flags & play_card_flags::sell_beer) && player->id == m_game->m_player_own_id) {
        m_targets.emplace_back(std::vector{target_pair{player, card}});
        send_play_card();
    } else if (m_game->m_current_request
        && (m_game->m_current_request->target_id == 0
        || m_game->m_current_request->target_id == m_game->m_player_own_id)) {
        if (!m_playing_card) {
            if (std::ranges::find(m_response_highlights, card) != m_response_highlights.end()) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            } else if (is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::player_hand)) {
                add_action<game_action_type::pick_card>(card_pile_type::player_hand, player->id, card->id);
            }
        } else {
            add_card_target(target_pair{player, card});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (player->id == m_game->m_player_own_id && m_game->has_player_flags(player_flags::has_drawn)) {
                if (card->color == card_color_type::brown) {
                    if (card->modifier != card_modifier_type::none) {
                        add_modifier(card);
                    } else if (verify_modifier(card)) {
                        m_playing_card = card;
                        handle_auto_targets();
                    }
                } else {
                    if (card->equip_targets.empty()
                        || card->equip_targets.front().target == enums::flags_none<target_type>
                        || bool(card->equip_targets.front().target & target_type::self)) {
                        add_action<game_action_type::play_card>(card->id, std::vector<int>{}, std::vector{
                            play_card_target{enums::enum_constant<play_card_target_type::target_player>{},
                            std::vector{target_player_id{player->id}}}
                        });
                    } else {
                        m_playing_card = card;
                        m_flags |= play_card_flags::equipping;
                    }
                }
            }
        } else {
            add_card_target(target_pair{player, card});
        }
    }
}

void target_finder::on_click_character(player_view *player, card_view *card) {
    m_flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (std::ranges::find(m_response_highlights, card) != m_response_highlights.end()) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            } else if (is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::player_character)) {
                add_action<game_action_type::pick_card>(card_pile_type::player_character, player->id, card->id);
            }
        } else {
            add_character_target(target_pair{player, card});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            const bool is_drawing_card = !card->targets.empty() && card->targets.front().type == effect_type::drawing;
            if (player->id == m_game->m_player_own_id && m_game->has_player_flags(player_flags::has_drawn) != is_drawing_card) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else if (!card->targets.empty()) {
                    m_playing_card = card;
                    handle_auto_targets();
                }
            }
        } else {
            add_character_target(target_pair{player, card});
        }
    }
}

void target_finder::on_click_scenario_card(card_view *card) {
    m_flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (m_game->m_playing_id == m_game->m_player_own_id
        && !m_playing_card && !card->targets.empty()) {
        bool is_drawing_card = !card->targets.empty() && card->targets.front().type == effect_type::drawing;
        if (m_game->has_player_flags(player_flags::has_drawn) != is_drawing_card) {
            m_playing_card = card;
            handle_auto_targets();
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    if (bool(m_flags & (play_card_flags::sell_beer | play_card_flags::discard_black))) return false;

    if (m_game->m_current_request && m_game->m_current_request->target_id == m_game->m_player_own_id) {
        if (is_valid_picking_pile(m_game->m_current_request->type, card_pile_type::player)) {
            add_action<game_action_type::pick_card>(card_pile_type::player, player->id);
            return true;
        } else if (m_playing_card) {
            return add_player_targets(std::vector{target_pair{player}});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id && m_playing_card) {
        return add_player_targets(std::vector{target_pair{player}});
    }
    return false;
}

void target_finder::on_click_sell_beer() {
    if (!bool(m_game->m_expansions & card_expansion_type::goldrush)) return;
    if (m_modifiers.empty() && !m_playing_card && m_game->m_playing_id == m_game->m_player_own_id && m_game->has_player_flags(player_flags::has_drawn)) {
        m_flags ^= play_card_flags::sell_beer;
        m_flags &= ~(play_card_flags::discard_black);
    }
}

void target_finder::on_click_discard_black() {
    if (!bool(m_game->m_expansions & card_expansion_type::goldrush)) return;
    if (m_modifiers.empty() && !m_playing_card && m_game->m_playing_id == m_game->m_player_own_id && m_game->has_player_flags(player_flags::has_drawn)) {
        m_flags ^= play_card_flags::discard_black;
        m_flags &= ~(play_card_flags::sell_beer);
    }
}

void target_finder::add_modifier(card_view *card) {
    if (m_modifiers.empty() || std::ranges::find(m_modifiers, card->modifier, &card_view::modifier) != m_modifiers.end()) {
        if (std::ranges::find(m_modifiers, card) == m_modifiers.end()) {
            m_modifiers.push_back(card);
        }
    }
}

bool target_finder::verify_modifier(card_view *card) {
    if (m_modifiers.empty()) return true;
    switch (m_modifiers.front()->modifier) {
    case card_modifier_type::bangcard:
        return std::ranges::find(card->targets, effect_type::bangcard, &card_target_data::type) != card->targets.end();
    case card_modifier_type::leevankliff:
        return !card->targets.empty() && card->targets.front().type == effect_type::bangcard
            && m_game->m_last_played_card;
    case card_modifier_type::discount:
        return card->expansion == card_expansion_type::goldrush;
    default:
        return false;
    }
}

std::vector<card_target_data> &target_finder::get_current_card_targets() {
    assert(!bool(m_flags & play_card_flags::equipping));

    card_view *card = nullptr;
    if (m_game->m_last_played_card && !m_modifiers.empty() && m_modifiers.front()->modifier == card_modifier_type::leevankliff) {
        card = m_game->m_last_played_card;
    } else {
        card = m_playing_card;
    }
    assert(card != nullptr);

    if (bool(m_flags & play_card_flags::response)) {
        return card->response_targets;
    } else {
        return card->targets;
    }
}

std::vector<card_target_data> &target_finder::get_optional_targets() {
    if (m_game->m_last_played_card && !m_modifiers.empty() && m_modifiers.front()->modifier == card_modifier_type::leevankliff) {
        return m_game->m_last_played_card->optional_targets;
    } else {
        return m_playing_card->optional_targets;
    }
}

target_type target_finder::get_target_type(int index) {
    auto &targets = get_current_card_targets();
    if (index < targets.size()) {
        return targets[index].target;
    }

    auto &optionals = get_optional_targets();
    return optionals[(index - targets.size()) % optionals.size()].target;
}

int target_finder::num_targets_for(target_type type) {
    if (bool(type & target_type::everyone)) {
        return std::ranges::count_if(m_game->m_players | std::views::values, [&](const player_view &p) {
            if (p.id == m_game->m_player_own_id && bool(type & target_type::notself)) return false;
            return !bool(type & target_type::card) || !(p.table.empty() && p.hand.empty());
        });
    }
    return 1;
};

int target_finder::get_target_index() {
    if (m_targets.empty()) return 0;
    int index = m_targets.size() - 1;
    index += m_targets[index].size() >= num_targets_for(get_target_type(index));
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
    if (from->has_player_flags(player_flags::see_everyone_range_1)) return 1;
    int d1=0, d2=0;
    for (player_view *counter = from; counter != to; counter = get_next_player(counter), ++d1);
    for (player_view *counter = to; counter != from; counter = get_next_player(counter), ++d2);
    return std::min(d1, d2) + to->m_distance_mod;
}

void target_finder::handle_auto_targets() {
    using namespace enums::flag_operators;

    auto *self_player = m_game->find_player(m_game->m_player_own_id);
    auto do_handle_target = [&](target_type type) {
        switch (type) {
        case enums::flags_none<target_type>:
            m_targets.push_back(std::vector{target_pair{}});
            return true;
        case target_type::self | target_type::player:
            m_targets.push_back(std::vector{target_pair{self_player}});
            return true;
        case target_type::self | target_type::card:
            m_targets.push_back(std::vector{target_pair{self_player, m_playing_card}});
            return true;
        case target_type::everyone | target_type::player: {
            std::vector<target_pair> ids;
            auto self = m_game->m_players.find(m_game->m_player_own_id);
            for (auto it = self;;) {
                if (!it->second.dead) {
                    ids.emplace_back(&it->second);
                }
                if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
                if (it == self) break;
            }
            m_targets.push_back(ids);
            return true;
        }
        case target_type::everyone | target_type::notself | target_type::player: {
            std::vector<target_pair> ids;
            auto self = m_game->m_players.find(m_game->m_player_own_id);
            for (auto it = self;;) {
                if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
                if (it == self) break;
                if (!it->second.dead) {
                    ids.emplace_back(&it->second);
                }
            }
            m_targets.push_back(ids);
            return true;
        }
        default:
            return false;
        }
    };

    auto &targets = get_current_card_targets();
    auto &optionals = get_optional_targets();
    bool repeatable = !optionals.empty() && optionals.back().type == effect_type::repeatable;
    if (targets.empty()) {
        clear_targets();
    } else {
        auto target_it = targets.begin() + m_targets.size();
        auto target_end = targets.end();
        if (target_it >= target_end && !optionals.empty()) {
            int diff = m_targets.size() - targets.size();
            target_it = optionals.begin() + (repeatable ? diff % optionals.size() : diff);
            target_end = optionals.end();
        }
        for(;;++target_it) {
            if (target_it == target_end) {
                if (optionals.empty() || (target_end == optionals.end() && !repeatable)) {
                    break;
                }
                target_it = optionals.begin();
                target_end = optionals.end();
            }
            if (!do_handle_target(target_it->target)) break;
        }
        if (target_it == target_end && (optionals.empty() || !repeatable)) {
            send_play_card();
        }
    }
}

constexpr auto is_player_target = [](const target_pair &pair) {
    return !pair.card;
};

constexpr auto is_none_target = [](const target_pair &pair) {
    return !pair.player && !pair.card;
};

void target_finder::add_card_target(target_pair target) {
    if (bool(m_flags & play_card_flags::equipping)) return;

    int index = get_target_index();
    auto cur_target = get_target_type(index);

    bool from_hand = target.card->pile == &target.player->hand;

    bool is_bang = !target.card->targets.empty() && target.card->targets.front().type == effect_type::bangcard;
    bool is_missed = !target.card->response_targets.empty() && target.card->response_targets.front().type == effect_type::missedcard;

    player_view *own_player = m_game->find_player(m_game->m_player_own_id);

    if (!std::ranges::all_of(util::enum_flag_values(cur_target),
        [&](target_type value) {
            switch (value) {
            case target_type::card:
            case target_type::everyone:
            case target_type::new_target:
            case target_type::can_repeat: return true;
            case target_type::reachable: return calc_distance(own_player, target.player) <= own_player->m_weapon_range + own_player->m_range_mod;
            case target_type::range_1: return calc_distance(own_player, target.player) <= 1 + own_player->m_range_mod;
            case target_type::range_2:return calc_distance(own_player, target.player) <= 2 + own_player->m_range_mod;
            case target_type::self: return target.player->id == m_game->m_player_own_id;
            case target_type::notself: return target.player->id != m_game->m_player_own_id;
            case target_type::notsheriff: return target.player && target.player->m_role.role != player_role::sheriff;
            case target_type::table: return !from_hand;
            case target_type::hand: return from_hand;
            case target_type::blue: return target.card->color == card_color_type::blue;
            case target_type::clubs: return target.card->suit == card_suit_type::clubs;
            case target_type::bang:
                return m_game->has_player_flags(player_flags::treat_any_as_bang)
                    || is_bang || (m_game->has_player_flags(player_flags::treat_missed_as_bang) && is_missed);
            case target_type::missed: return is_missed;
            case target_type::cube_slot: return target.card->color == card_color_type::orange;
            default: return false;
            }
        })) return;
    
    if (bool(cur_target & target_type::card)) {
        if ((bool(cur_target & target_type::can_repeat)
            || std::ranges::none_of(m_targets, [&](const auto &vec) {
                return std::ranges::find(vec, target.card, &target_pair::card) != vec.end();
            }))
            && target.card != m_playing_card
            && std::ranges::find(m_modifiers, target.card) == m_modifiers.end())
        {
            auto &l = index >= m_targets.size()
                ? m_targets.emplace_back()
                : m_targets.back();
            l.push_back(target);
            if (l.size() == num_targets_for(cur_target)) {
                handle_auto_targets();
            }
        }
    } else if (bool(cur_target & target_type::cube_slot)) {
        int ncubes = std::ranges::count(m_selected_cubes, target.card, &cube_widget::owner);
        if (ncubes < target.card->cubes.size()) {
            auto &l = index >= m_targets.size()
                ? m_targets.emplace_back()
                : m_targets.back();
            l.push_back(target);
            m_selected_cubes.push_back(*(target.card->cubes.rbegin() + ncubes));
            handle_auto_targets();
        }
    }
}

void target_finder::add_character_target(target_pair target) {
    if (bool(m_flags & play_card_flags::equipping)) return;
    
    auto type = get_target_type(get_target_index());
    if (!bool(type & target_type::cube_slot)) return;
    if (bool(type & target_type::table)) return;

    if (std::ranges::find(target.player->m_characters, target.card) != target.player->m_characters.end()) {
        target.card = target.player->m_characters.front();
    } else {
        return;
    }

    if(bool(type & target_type::card)) {
        m_targets.emplace_back(std::vector{target});
        handle_auto_targets();
    } else {
        int ncubes = std::ranges::count(m_selected_cubes, target.card, &cube_widget::owner);
        if (ncubes < target.card->cubes.size()) {
            m_targets.emplace_back(std::vector{target});
            m_selected_cubes.push_back(*(target.card->cubes.rbegin() + ncubes));
            handle_auto_targets();
        }
    }
}

bool target_finder::add_player_targets(const std::vector<target_pair> &targets) {
    auto verify_target = [&](target_type type) {
        player_view *own_player = m_game->find_player(m_game->m_player_own_id);
        return std::ranges::all_of(targets, [&](player_view *target_player) {
            return std::ranges::all_of(util::enum_flag_values(type), [&](target_type value){
                switch(value) {
                case target_type::player: return !target_player->dead;
                case target_type::dead: return target_player->dead;
                case target_type::self: return target_player->id == m_game->m_player_own_id;
                case target_type::notself: return target_player->id != m_game->m_player_own_id;
                case target_type::notsheriff: return target_player->m_role.role != player_role::sheriff;
                case target_type::new_target:
                    return std::ranges::none_of(m_targets, [&](const auto &vec) {
                        return std::ranges::find(vec, target_player, &target_pair::player) != vec.end();
                    });
                case target_type::reachable: return calc_distance(own_player, target_player) <= own_player->m_weapon_range + own_player->m_range_mod;
                case target_type::range_1: return calc_distance(own_player, target_player) <= 1 + own_player->m_range_mod;
                case target_type::range_2: return calc_distance(own_player, target_player) <= 2 + own_player->m_range_mod;
                case target_type::fanning_target:
                    return !m_targets.empty() && !m_targets.back().empty()
                        && calc_distance(m_targets.back().back().player, target_player) <= 1;
                case target_type::everyone:
                    return true;
                default:
                    return false;
                }
            });
        }, &target_pair::player);
    };
    if (bool(m_flags & play_card_flags::equipping)) {
        if (verify_target(m_playing_card->equip_targets[m_targets.size()].target)) {
            m_targets.emplace_back(targets);
            send_play_card();
            return true;
        }
    } else {
        if (verify_target(get_target_type(get_target_index()))) {
            m_targets.emplace_back(targets);
            handle_auto_targets();
            return true;
        }
    }
    return false;
}

void target_finder::clear_targets() {
    static_cast<target_status &>(*this) = {};
}

void target_finder::send_play_card() {
    play_card_args ret;
    ret.card_id = m_playing_card ? m_playing_card->id : 0;
    for (card_view *card : m_modifiers) {
        ret.modifier_ids.push_back(card->id);
    }
    ret.flags = m_flags;

    for (const auto &vec : m_targets) {
        if (std::ranges::all_of(vec, is_none_target)) {
            ret.targets.emplace_back(enums::enum_constant<play_card_target_type::target_none>());
        } else if (std::ranges::all_of(vec, is_player_target)) {
            auto &subvec = ret.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>())
                .get<play_card_target_type::target_player>();
            for (const auto &pair : vec) {
                subvec.emplace_back(pair.player->id);
            }
        } else {
            auto &subvec = ret.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>())
                .get<play_card_target_type::target_card>();
            for (const auto &pair : vec) {
                subvec.emplace_back(pair.player->id, pair.card->id);
            }
        }
    }
    add_action<game_action_type::play_card>(std::move(ret));
    clear_targets();
}