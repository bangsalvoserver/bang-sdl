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
    constexpr auto lut = []<request_type ... Es>(enums::enum_sequence<Es...>) {
        return std::array {
            +[](card_pile_type pile) {
                if constexpr (picking_request<Es>) {
                    return enums::enum_type_t<Es>::valid_pile(pile);
                } else {
                    return false;
                }
            } ...
        };
    }(enums::make_enum_sequence<request_type>());
    return lut[enums::indexof(type)](pile);
}

void target_finder::render(sdl::renderer &renderer) {
    renderer.set_draw_color(sdl::color{0xff, 0x0, 0x0, 0xff});
    for (auto *card : m_modifiers) {
        renderer.draw_rect(card->get_rect());
    }
    if (m_playing_card) {
        renderer.draw_rect(m_playing_card->get_rect());
    }
    renderer.set_draw_color(sdl::color{0xff, 0x0, 0xff, 0xff});
    for (auto &l : m_targets) {
        for (auto [player, card] : l) {
            if (card) {
                renderer.draw_rect(card->get_rect());
            } else if (player) {
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
    if (m_game->m_current_request.target_id == m_game->m_player_own_id
        && is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::main_deck)) {
        add_action<game_action_type::pick_card>(card_pile_type::main_deck);
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        auto *player = m_game->find_player(m_game->m_playing_id);
        if (auto it = std::ranges::find(player->m_characters, character_type::drawing_forced, &character_card::type); it != player->m_characters.end()) {
            on_click_character(m_game->find_player(m_game->m_playing_id), &*it);
        } else {
            add_action<game_action_type::draw_from_deck>();
        }
    }
}

void target_finder::on_click_selection_card(card_view *card) {
    if (m_game->m_current_request.target_id == m_game->m_player_own_id
        && is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::selection)) {
        add_action<game_action_type::pick_card>(card_pile_type::selection, 0, card->id);
    }
}

void target_finder::on_click_shop_card(card_view *card) {
    m_flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (!m_playing_card) {
        if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
            if (card->color == card_color_type::none) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            }
        } else if (m_game->m_playing_id == m_game->m_player_own_id) {
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
    } else if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::player_table)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_table, player->id, card->id);
        } else if (!m_playing_card) {
            if (!card->inactive) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            }
        } else {
            m_flags |= play_card_flags::response;
            add_card_target(target_pair{player, card});
        }
    } else if (!m_playing_card && card->playable_offturn && !card->inactive) {
        m_playing_card = card;
        m_flags |= play_card_flags::response | play_card_flags::offturn;
        handle_auto_targets();
    } else if (bool(m_flags & play_card_flags::offturn)) {
        m_flags |= play_card_flags::response;
        add_card_target(target_pair{player, card});
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (player->id == m_game->m_player_own_id && !card->inactive) {
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

bool target_finder::is_escape_card(card_widget *card) {
    if (std::ranges::find(card->response_targets, effect_type::escape, &card_target_data::type) != card->response_targets.end()) {
        return bool(m_game->m_current_request.flags & effect_flags::escapable);
    }
    return false;
}

void target_finder::on_click_hand_card(player_view *player, card_view *card) {
    m_flags &= ~play_card_flags::discard_black;

    if (bool(m_flags & play_card_flags::sell_beer) && player->id == m_game->m_player_own_id) {
        m_targets.emplace_back(std::vector{target_pair{player, card}});
        send_play_card();
    } else if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::player_hand) && !is_escape_card(card)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_hand, player->id, card->id);
        } else if (!m_playing_card) {
            m_playing_card = card;
            m_flags |= play_card_flags::response;
            handle_auto_targets();
        } else {
            m_flags |= play_card_flags::response;
            add_card_target(target_pair{player, card});
        }
    } else if (!m_playing_card && card->playable_offturn && card->color == card_color_type::brown) {
        m_playing_card = card;
        m_flags |= play_card_flags::response | play_card_flags::offturn;
        handle_auto_targets();
    } else if (bool(m_flags & play_card_flags::offturn)) {
        m_flags |= play_card_flags::response;
        add_card_target(target_pair{player, card});
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (player->id == m_game->m_player_own_id) {
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

void target_finder::on_click_character(player_view *player, character_card *card) {
    m_flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::player_character) && !is_escape_card(card)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_character, player->id, card->id);
        } else if (player->id == m_game->m_player_own_id) {
            if (!m_playing_card) {
                m_playing_card = card;
                m_flags |= play_card_flags::response;
                handle_auto_targets();
            } else {
                m_flags |= play_card_flags::response;
                add_character_target(target_pair{player, card});
            }
        }
    } else if (card->playable_offturn && !m_playing_card && player->id == m_game->m_player_own_id) {
        m_playing_card = card;
        m_flags |= play_card_flags::response | play_card_flags::offturn;
        handle_auto_targets();
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (!m_playing_card) {
            if (player->id == m_game->m_player_own_id && card->type != character_type::none) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else {
                    m_playing_card = card;
                    handle_auto_targets();
                }
            }
        } else {
            add_character_target(target_pair{player, card});
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    if (bool(m_flags & (play_card_flags::sell_beer | play_card_flags::discard_black))) return false;

    if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_valid_picking_pile(m_game->m_current_request.type, card_pile_type::player)) {
            add_action<game_action_type::pick_card>(card_pile_type::player, player->id);
            return true;
        } else if (m_playing_card) {
            m_flags |= play_card_flags::response;
            return add_player_targets(std::vector{target_pair{player}});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id && m_playing_card) {
        return add_player_targets(std::vector{target_pair{player}});
    }
    return false;
}

void target_finder::on_click_sell_beer() {
    if (!bool(m_game->m_expansions & card_expansion_type::goldrush)) return;
    if (m_modifiers.empty() && !m_playing_card && m_game->m_playing_id == m_game->m_player_own_id) {
        m_flags ^= play_card_flags::sell_beer;
        m_flags &= ~(play_card_flags::discard_black);
    }
}

void target_finder::on_click_discard_black() {
    if (!bool(m_game->m_expansions & card_expansion_type::goldrush)) return;
    if (m_modifiers.empty() && !m_playing_card && m_game->m_playing_id == m_game->m_player_own_id) {
        m_flags ^= play_card_flags::discard_black;
        m_flags &= ~(play_card_flags::sell_beer);
    }
}

void target_finder::add_modifier(card_widget *card) {
    if (m_modifiers.empty() || std::ranges::find(m_modifiers, card->modifier, &card_widget::modifier) != m_modifiers.end()) {
        if (std::ranges::find(m_modifiers, card) == m_modifiers.end()) {
            m_modifiers.push_back(card);
        }
    }
}

bool target_finder::verify_modifier(card_widget *card) {
    if (m_modifiers.empty()) return true;
    switch (m_modifiers.front()->modifier) {
    case card_modifier_type::bangcard:
        return std::ranges::find(card->targets, effect_type::bangcard, &card_target_data::type) != card->targets.end();
    case card_modifier_type::discount:
        return card->expansion == card_expansion_type::goldrush;
    default:
        return false;
    }
}

std::vector<card_target_data> &target_finder::get_current_card_targets() {
    assert(!bool(m_flags & play_card_flags::equipping));

    card_widget *card = nullptr;
    if (m_virtual) {
        card = &*m_virtual;
    } else if (m_playing_card) {
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
    if (m_virtual) {
        return m_virtual->optional_targets;
    } else {
        return m_playing_card->optional_targets;
    }
}

card_target_data &target_finder::get_target_at(int index) {
    auto &targets = get_current_card_targets();
    if (index < targets.size()) {
        return targets[index];
    }

    auto &optionals = get_optional_targets();
    return optionals[(index - targets.size()) % optionals.size()];
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
        case target_type::attacker | target_type::player:
            m_targets.push_back(std::vector{target_pair{m_game->find_player(m_game->m_current_request.origin_id)}});
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
    bool repeatable = m_virtual ? m_virtual->optional_repeatable : m_playing_card->optional_repeatable;
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
    
    auto num_targets = [&](target_type type) -> int {
        if (bool(type & target_type::everyone)) {
            return std::ranges::count_if(m_game->m_players | std::views::values, [&](const player_view &p) {
                if (p.id == m_game->m_player_own_id && bool(type & target_type::notself)) return false;
                return !(p.table.empty() && p.hand.empty());
            });
        }
        return 1;
    };

    std::cout << "AAA" << std::endl;

    auto &card_targets = get_current_card_targets();
    int index = std::max(0, int(m_targets.size()) - 1);

    auto cur_target = get_target_at(index).target;
    if (!m_targets.empty() && m_targets[index].size() >= num_targets(cur_target)) {
        ++index;
        cur_target = get_target_at(index).target;
    }

    std::cout << "index = " << index
        // << ", cur_target = " << enums::flags_to_string(cur_target)
        << ", num_targets = " << num_targets(cur_target) << std::endl;

    card_view *as_deck_card = target.card ? m_game->find_card(target.card->id) : nullptr;
    bool from_hand = std::ranges::find(target.player->hand, as_deck_card) != target.player->hand.end();

    bool is_bang = as_deck_card && !as_deck_card->targets.empty() && as_deck_card->targets.front().type == effect_type::bangcard;
    bool is_missed = as_deck_card && !as_deck_card->response_targets.empty() && as_deck_card->response_targets.front().type == effect_type::missedcard;

    if (!std::ranges::all_of(util::enum_flag_values(cur_target),
        [&](target_type value) {
            switch (value) {
            case target_type::card:
            case target_type::everyone:
            case target_type::reachable:
            case target_type::maxdistance:
            case target_type::new_target:
            case target_type::can_repeat: return true;
            case target_type::self: return target.player->id == m_game->m_player_own_id;
            case target_type::notself: return target.player->id != m_game->m_player_own_id;
            case target_type::notsheriff: return target.player && target.player->m_role.role != player_role::sheriff;
            case target_type::table: return !from_hand;
            case target_type::hand: return from_hand;
            case target_type::blue: return as_deck_card && as_deck_card->color == card_color_type::blue;
            case target_type::clubs: return as_deck_card && as_deck_card->suit == card_suit_type::clubs;
            case target_type::bang: return is_bang;
            case target_type::missed: return is_missed;
            case target_type::bangormissed: return is_bang || is_missed;
            case target_type::cube_slot: return as_deck_card && as_deck_card->color == card_color_type::orange;
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
            if (l.size() == num_targets(cur_target)) {
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
    
    auto &card_targets = get_current_card_targets();
    auto &cur_target = get_target_at(m_targets.size());
    if (!bool(cur_target.target & target_type::cube_slot)) return;
    if (bool(cur_target.target & target_type::table)) return;

    character_card *card = nullptr;
    for (auto &p : m_game->m_players | std::views::values) {
        if (&p.m_characters.front() == target.card) {
            card = &p.m_characters.front();
            break;
        }
    }
    if (!card) return;

    if(bool(cur_target.target & target_type::card)) {
        m_targets.emplace_back(std::vector{target});
        handle_auto_targets();
    } else {
        int ncubes = std::ranges::count(m_selected_cubes, card, &cube_widget::owner);
        if (ncubes < card->cubes.size()) {
            m_targets.emplace_back(std::vector{target});
            m_selected_cubes.push_back(*(card->cubes.rbegin() + ncubes));
            handle_auto_targets();
        }
    }
}

bool target_finder::add_player_targets(const std::vector<target_pair> &targets) {
    if (bool(m_flags & play_card_flags::equipping)) {
        auto &card_targets = m_playing_card->equip_targets;
        auto &cur_target = card_targets[m_targets.size()];
        if (bool(cur_target.target & (target_type::player | target_type::dead))) {
            m_targets.emplace_back(targets);
            send_play_card();
            return true;
        }
    } else {
        auto &card_targets = get_current_card_targets();
        auto &cur_target = get_target_at(m_targets.size());
        if (bool(cur_target.target & (target_type::player | target_type::dead))) {
            if (!bool(cur_target.target & target_type::new_target)
                || std::ranges::none_of(m_targets, [&](const auto &vec) {
                    return std::ranges::any_of(vec, [&](const target_pair &pair) {
                        return std::ranges::find(targets, pair.player, &target_pair::player) != targets.end();
                    });
                }))
            {
                m_targets.emplace_back(targets);
                handle_auto_targets();
                return true;
            }
        }
    }
    return false;
}

void target_finder::clear_targets() {
    static_cast<target_status &>(*this) = {};
}

void target_finder::handle_virtual_card(const virtual_card_update &args) {
    auto &c = m_virtual.emplace();
    c.id = args.virtual_id;
    c.suit = args.suit;
    c.value = args.value;
    c.color = args.color;
    c.targets = args.targets;

    m_playing_card = m_game->find_card_widget(args.card_id);
    handle_auto_targets();
}

void target_finder::send_play_card() {
    play_card_args ret;
    if (m_virtual) {
        ret.card_id = m_virtual->id;
    } else {
        ret.card_id = m_playing_card ? m_playing_card->id : 0;
    }
    for (card_widget *card : m_modifiers) {
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