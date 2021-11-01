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

void target_finder::render(sdl::renderer &renderer) {
    renderer.set_draw_color(sdl::color{0xff, 0x0, 0x0, 0xff});
    for (card_widget *card : m_highlights) {
        renderer.draw_rect(card->get_rect());
    }
}

void target_finder::on_click_main_deck() {
    if (m_game->m_current_request.target_id == m_game->m_player_own_id && is_picking_request(m_game->m_current_request.type)) {
        add_action<game_action_type::pick_card>(card_pile_type::main_deck);
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        auto *player = m_game->find_player(m_game->m_playing_id);
        if (auto it = std::ranges::find(player->m_characters, character_type::drawing_forced, &character_card::type); it != player->m_characters.end()) {
            on_click_character(m_game->m_playing_id, &*it);
        } else {
            add_action<game_action_type::draw_from_deck>();
        }
    }
}

void target_finder::on_click_selection_card(card_view *card) {
    if (m_game->m_current_request.target_id == m_game->m_player_own_id && is_picking_request(m_game->m_current_request.type)) {
        add_action<game_action_type::pick_card>(card_pile_type::selection, 0, card->id);
    }
}

void target_finder::on_click_shop_card(card_view *card) {
    m_play_card_args.flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (!m_highlights.empty() && card == m_highlights.front()) {
        clear_targets();
    } else if (m_play_card_args.card_id == 0) {
        if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
            if (card->color == card_color_type::none) {
                m_play_card_args.card_id = card->id;
                m_highlights.push_back(card);
                handle_auto_targets(true);
            }
        } else if (m_game->m_playing_id == m_game->m_player_own_id) {
            if (!verify_modifier(card)) return;
            if (card->color == card_color_type::black) {
                if (card->equip_targets.empty()
                    || card->equip_targets.front().target == enums::flags_none<target_type>
                    || bool(card->equip_targets.front().target & target_type::self)) {
                    m_play_card_args.card_id = card->id;
                    m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{},
                        std::vector{target_player_id{m_game->m_playing_id}});
                    add_action<game_action_type::play_card>(m_play_card_args);
                    clear_targets();
                } else {
                    m_play_card_args.card_id = card->id;
                    m_play_card_args.flags |= play_card_flags::equipping;
                    m_highlights.push_back(card);
                }
            } else {
                m_play_card_args.card_id = card->id;
                m_highlights.push_back(card);
                handle_auto_targets(false);
            }
        }
    }
}

void target_finder::on_click_table_card(int player_id, card_view *card) {
    m_play_card_args.flags &= ~play_card_flags::sell_beer;

    if (bool(m_play_card_args.flags & play_card_flags::discard_black)) {
        m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{},
            std::vector{target_card_id{player_id, card->id, false}});
        add_action<game_action_type::play_card>(m_play_card_args);
        clear_targets();
    } else if (!m_highlights.empty() && card == m_highlights.front()) {
        clear_targets();
    } else if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_picking_request(m_game->m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_table, player_id, card->id);
        } else if (m_play_card_args.card_id == 0) {
            if (!card->inactive) {
                m_play_card_args.card_id = card->id;
                m_highlights.push_back(card);
                handle_auto_targets(true);
            }
        } else {
            add_card_target(true, target_card_id{player_id, card->id, false});
        }
    } else if (m_play_card_args.card_id == 0 && card->playable_offturn && !card->inactive) {
        m_play_card_args.card_id = card->id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card);
        handle_auto_targets(true);
    } else if (bool(m_play_card_args.flags & play_card_flags::offturn)) {
        add_card_target(true, target_card_id{player_id, card->id, false});
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (m_play_card_args.card_id == 0) {
            if (player_id == m_game->m_player_own_id && !card->inactive && verify_modifier(card)) {
                m_highlights.push_back(card);
                if (card->modifier != card_modifier_type::none) {
                    m_play_card_args.modifier_id = card->id;
                } else {
                    m_play_card_args.card_id = card->id;
                    handle_auto_targets(false);
                }
            }
        } else {
            add_card_target(false, target_card_id{player_id, card->id, false});
        }
    }
}

void target_finder::on_click_hand_card(int player_id, card_view *card) {
    m_play_card_args.flags &= ~play_card_flags::discard_black;

    if (bool(m_play_card_args.flags & play_card_flags::sell_beer) && player_id == m_game->m_player_own_id) {
        m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{},
            std::vector{target_card_id{player_id, card->id, true}});
        add_action<game_action_type::play_card>(m_play_card_args);
        clear_targets();
    } else if (!m_highlights.empty() && card == m_highlights.front()) {
        clear_targets();
    } else if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_picking_request(m_game->m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_hand, player_id, card->id);
        } else if (m_play_card_args.card_id == 0) {
            m_play_card_args.card_id = card->id;
            m_highlights.push_back(card);
            handle_auto_targets(true);
        } else {
            add_card_target(true, target_card_id{player_id, card->id, true});
        }
    } else if (m_play_card_args.card_id == 0 && card->playable_offturn) {
        m_play_card_args.card_id = card->id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card);
        handle_auto_targets(true);
    } else if (bool(m_play_card_args.flags & play_card_flags::offturn)) {
        add_card_target(true, target_card_id{player_id, card->id, true});
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (m_play_card_args.card_id == 0) {
            if (player_id == m_game->m_player_own_id && verify_modifier(card)) {
                if (card->color == card_color_type::brown) {
                    m_highlights.push_back(card);
                    if (card->modifier != card_modifier_type::none) {
                        m_play_card_args.modifier_id = card->id;
                    } else {
                        m_play_card_args.card_id = card->id;
                        handle_auto_targets(false);
                    }
                } else {
                    if (card->equip_targets.empty()
                        || card->equip_targets.front().target == enums::flags_none<target_type>
                        || bool(card->equip_targets.front().target & target_type::self)) {
                        add_action<game_action_type::play_card>(card->id, 0, std::vector{
                            play_card_target{enums::enum_constant<play_card_target_type::target_player>{},
                            std::vector{target_player_id{player_id}}}
                        });
                    } else {
                        m_play_card_args.card_id = card->id;
                        m_play_card_args.flags |= play_card_flags::equipping;
                        m_highlights.push_back(card);
                    }
                }
            }
        } else {
            add_card_target(false, target_card_id{player_id, card->id, true});
        }
    }
}

void target_finder::on_click_character(int player_id, character_card *card) {
    m_play_card_args.flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (!m_highlights.empty() && card == m_highlights.front()) {
        clear_targets();
    } else if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_picking_request(m_game->m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_character, player_id, card->id);
        } else if (m_play_card_args.card_id == 0 && player_id == m_game->m_player_own_id) {
            m_play_card_args.card_id = card->id;
            m_highlights.push_back(card);
            handle_auto_targets(true);
        }
    } else if (card->playable_offturn && m_play_card_args.card_id == 0 && player_id == m_game->m_player_own_id) {
        m_play_card_args.card_id = card->id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card);
        handle_auto_targets(true);
    } else if (m_game->m_playing_id == m_game->m_player_own_id) {
        if (m_play_card_args.card_id == 0) {
            if (player_id == m_game->m_player_own_id && card->type != character_type::none) {
                m_highlights.push_back(card);
                if (card->modifier != card_modifier_type::none) {
                    m_play_card_args.modifier_id = card->id;
                } else {
                    m_play_card_args.card_id = card->id;
                    handle_auto_targets(false);
                }
            }
        } else {
            add_character_target(false, target_card_id{player_id, card->id, false});
        }
    }
}

void target_finder::on_click_player(int player_id) {
    m_play_card_args.flags &= ~(play_card_flags::sell_beer | play_card_flags::discard_black);

    if (m_game->m_current_request.target_id == m_game->m_player_own_id && m_game->m_current_request.type != request_type::none) {
        if (is_picking_request(m_game->m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player, player_id);
        } else if (m_play_card_args.card_id != 0) {
            add_player_targets(true, std::vector{target_player_id{player_id}});
        }
    } else if (m_game->m_playing_id == m_game->m_player_own_id && m_play_card_args.card_id != 0) {
        add_player_targets(false, std::vector{target_player_id{player_id}});
    }
}

void target_finder::on_click_sell_beer() {
    if (m_play_card_args.card_id == 0 && m_game->m_playing_id == m_game->m_player_own_id) {
        m_play_card_args.flags ^= play_card_flags::sell_beer;
        m_play_card_args.flags &= ~(play_card_flags::discard_black);
    }
}

void target_finder::on_click_discard_black() {
    if (m_play_card_args.card_id == 0 && m_game->m_playing_id == m_game->m_player_own_id) {
        m_play_card_args.flags ^= play_card_flags::discard_black;
        m_play_card_args.flags &= ~(play_card_flags::sell_beer);
    }
}

bool target_finder::verify_modifier(card_widget *card) {
    if (m_play_card_args.modifier_id == 0) return true;
    switch (m_game->find_card_widget(m_play_card_args.modifier_id)->modifier) {
    case card_modifier_type::bangcard:
        return std::ranges::find(card->targets, effect_type::bangcard, &card_target_data::type) != card->targets.end();
    case card_modifier_type::discount:
        return card->expansion == card_expansion_type::goldrush;
    default:
        return false;
    }
}



std::vector<card_target_data> &target_finder::get_current_card_targets(bool is_response) {
    assert(!bool(m_play_card_args.flags & play_card_flags::equipping));

    card_widget *card = nullptr;
    if (m_virtual) {
        card = &*m_virtual;
    } else if (m_play_card_args.card_id) {
        card = m_game->find_card_widget(m_play_card_args.card_id);
    } else if (m_play_card_args.modifier_id) {
        card = m_game->find_card_widget(m_play_card_args.modifier_id);
    }
    if (!card) throw std::runtime_error("Carta non trovata");
    if (is_response) {
        return card->response_targets;
    } else {
        return card->targets;
    }
}

void target_finder::handle_auto_targets(bool is_response) {
    using namespace enums::flag_operators;
    auto do_handle_target = [&](target_type type) {
        switch (type) {
        case enums::flags_none<target_type>:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_none>{});
            return true;
        case target_type::self | target_type::player:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{},
                std::vector{target_player_id{m_game->m_player_own_id}});
            return true;
        case target_type::self | target_type::card:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{},
                std::vector{target_card_id{m_game->m_player_own_id, m_play_card_args.card_id, false}});
            return true;
        case target_type::attacker | target_type::player:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{},
                std::vector{target_player_id{m_game->m_current_request.origin_id}});
            return true;
        case target_type::everyone | target_type::player: {
            std::vector<target_player_id> ids;
            auto self = m_game->m_players.find(m_game->m_player_own_id);
            for (auto it = self;;) {
                if (!it->second.dead) {
                    ids.emplace_back(it->first);
                }
                if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
                if (it == self) break;
            }
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{}, ids);
            return true;
        }
        case target_type::everyone | target_type::notself | target_type::player: {
            std::vector<target_player_id> ids;
            auto self = m_game->m_players.find(m_game->m_player_own_id);
            for (auto it = self;;) {
                if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
                if (it == self) break;
                if (!it->second.dead) {
                    ids.emplace_back(it->first);
                }
            }
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{}, ids);
            return true;
        }
        default:
            return false;
        }
    };

    auto &targets = get_current_card_targets(is_response);
    if (targets.empty()) {
        clear_targets();
    } else {
        auto target_it = targets.begin() + m_play_card_args.targets.size();
        for (; target_it != targets.end(); ++target_it) {
            if (!do_handle_target(target_it->target)) break;
        }
        if (target_it == targets.end()) {
            if (is_response) {
                add_action<game_action_type::respond_card>(m_play_card_args);
            } else {
                add_action<game_action_type::play_card>(m_play_card_args);
            }
            clear_targets();
        }
    }
}

void target_finder::add_card_target(bool is_response, const target_card_id &target) {
    if (std::ranges::find(m_highlights, target.card_id, [](const card_widget *w) { return w->id; }) != m_highlights.end()) return;

    auto &card_targets = get_current_card_targets(is_response);
    if (m_play_card_args.targets.empty()) {
        m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{});
    }

    auto cur_target = [&] {
        return card_targets[m_play_card_args.targets.size() - 1].target;
    };

    auto num_targets = [&] {
        int ret = 1;
        if (bool(cur_target() & target_type::everyone)) {
            ret = 0;
            for (const auto &p : m_game->m_players | std::views::values) {
                if (!p.dead) ++ret;
            }
            if (bool(cur_target() & target_type::notself)) {
                --ret;
            }
        }
        return ret;
    };

    auto is_bang = [](card_view *card) {
        return card && !card->targets.empty() && card->targets.front().type == effect_type::bangcard;
    };

    auto is_missed = [](card_view *card) {
        return card && !card->response_targets.empty() && card->response_targets.front().type == effect_type::missedcard;
    };

    if (m_play_card_args.targets.back().enum_index() != play_card_target_type::target_card
        || m_play_card_args.targets.back().get<play_card_target_type::target_card>().size() == num_targets()) {
        m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{});
    }

    player_view *player = m_game->find_player(target.player_id);
    card_view *card = m_game->find_card(target.card_id);

    if (!bool(cur_target() & target_type::card)) {
        m_play_card_args.targets.pop_back();
    } else if (std::ranges::all_of(enums::enum_values_v<target_type>
        | std::views::filter([&](target_type value) {
            return bool(cur_target() & value);
        }), [&](target_type value) {
            switch (value) {
            case target_type::card:
            case target_type::everyone:
            case target_type::reachable:
            case target_type::maxdistance:
            case target_type::new_target: return true;
            case target_type::self: return target.player_id == m_game->m_player_own_id;
            case target_type::notself: return target.player_id != m_game->m_player_own_id;
            case target_type::notsheriff: return player && player->m_role.role != player_role::sheriff;
            case target_type::table: return !target.from_hand;
            case target_type::hand: return target.from_hand;
            case target_type::blue: return card && card->color == card_color_type::blue;
            case target_type::clubs: return card && card->suit == card_suit_type::clubs;
            case target_type::bang: return is_bang(card);
            case target_type::missed: return is_missed(card);
            case target_type::bangormissed: return is_bang(card) || is_missed(card);
            case target_type::cube_slot: return card && card->color == card_color_type::orange;
            default: return false;
            }
        }))
    {
        m_highlights.push_back(m_game->find_card_widget(target.card_id));
        auto &l = m_play_card_args.targets.back().get<play_card_target_type::target_card>();
        l.push_back(target);
        if (l.size() == num_targets()) {
            handle_auto_targets(is_response);
        }
    }
}

void target_finder::add_character_target(bool is_response, const target_card_id &target) {
    if (bool(m_play_card_args.flags & play_card_flags::equipping)) return;
    
    auto &card_targets = get_current_card_targets(is_response);
    auto &cur_target = card_targets[m_play_card_args.targets.size()];
    if (bool(cur_target.target & target_type::cube_slot)) {
        character_card *card = nullptr;
        for (auto &p : m_game->m_players | std::views::values) {
            if (p.m_characters.front().id == target.card_id) {
                card = &p.m_characters.front();
                break;
            }
        }
        if (card) {
            m_highlights.push_back(card);
            m_play_card_args.targets.emplace_back(
                enums::enum_constant<play_card_target_type::target_card>{}, std::vector{target});
            handle_auto_targets(is_response);
        }
    }
}

void target_finder::add_player_targets(bool is_response, const std::vector<target_player_id> &targets) {
    if (bool(m_play_card_args.flags & play_card_flags::equipping)) {
        auto &card_targets = m_game->find_card_widget(m_play_card_args.card_id)->equip_targets;
        auto &cur_target = card_targets[m_play_card_args.targets.size()];
        if (bool(cur_target.target & (target_type::player | target_type::dead))) {
            m_play_card_args.targets.emplace_back(
                enums::enum_constant<play_card_target_type::target_player>{}, targets);
            add_action<game_action_type::play_card>(m_play_card_args);
            clear_targets();
        }
    } else {
        auto &card_targets = get_current_card_targets(is_response);
        auto &cur_target = card_targets[m_play_card_args.targets.size()];
        if (bool(cur_target.target & (target_type::player | target_type::dead))) {
            for (auto &t : targets) {
                m_highlights.push_back(&m_game->find_player(t.player_id)->m_role);
            }
            m_play_card_args.targets.emplace_back(
                enums::enum_constant<play_card_target_type::target_player>{}, targets);
            handle_auto_targets(is_response);
        }
    }
}

void target_finder::clear_targets() {
    m_play_card_args.targets.clear();
    m_play_card_args.card_id = 0;
    m_play_card_args.modifier_id = 0;
    m_play_card_args.flags = enums::flags_none<play_card_flags>;
    m_highlights.clear();
    m_virtual.reset();
}

void target_finder::handle_virtual_card(const virtual_card_update &args) {
    auto &c = m_virtual.emplace();
    c.id = args.virtual_id;
    c.suit = args.suit;
    c.value = args.value;
    c.color = args.color;
    c.targets = args.targets;

    m_highlights.push_back(m_game->find_card_widget(args.card_id));
    m_play_card_args.card_id = args.virtual_id;
    handle_auto_targets(false);
}