#include "game.h"
#include "../../manager.h"

#include <iostream>
#include <numbers>

using namespace banggame;
using namespace enums::flag_operators;

template<game_action_type T, typename ... Ts>
void game_scene::add_action(Ts && ... args) {
    parent->add_message<client_message_type::game_action>(enums::enum_constant<T>{}, std::forward<Ts>(args) ...);
}

void game_scene::add_pass_action() {
    add_action<game_action_type::pass_turn>();
}

void game_scene::add_resolve_action() {
    add_action<game_action_type::resolve>();
}

game_scene::game_scene(class game_manager *parent)
    : scene_base(parent)
    , m_ui(this) {}

void game_scene::resize(int width, int height) {
    scene_base::resize(width, height);
    
    main_deck.pos = sdl::point{parent->width() / 2, parent->height() / 2};

    discard_pile.pos = main_deck.pos;
    discard_pile.pos.x -= 80;
    
    temp_table.pos = sdl::point{
        (main_deck.pos.x + discard_pile.pos.x) / 2,
        main_deck.pos.y + 100};

    move_player_views();
    for (auto &[id, card] : m_cards) {
        card.pos = card.pile->get_position(card.id);
    }

    m_ui.resize(width, height);
}

void game_scene::render(sdl::renderer &renderer) {
    if (m_animations.empty()) {
        pop_update();
    } else {
        m_animations.front().tick();
        if (m_animations.front().done()) {
            m_animations.pop_front();
            pop_update();
        }
    }
    
    if (!m_player_own_id) return;

    for (int id : main_deck | std::views::reverse | std::views::take(2) | std::views::reverse) {
        get_card(id).render(renderer);
    }

    for (auto &[player_id, p] : m_players) {
        if (player_id == m_playing_id) {
            p.render_turn_indicator(renderer);
        }
        if (m_current_request.type != request_type::none && m_current_request.target_id == player_id) {
            p.render_request_indicator(renderer);
        }

        p.render(renderer);

        for (int id : p.table) {
            get_card(id).render(renderer);
        }
        for (int id : p.hand) {
            get_card(id).render(renderer);
        }
    }

    for (int id : discard_pile | std::views::reverse | std::views::take(2) | std::views::reverse) {
        get_card(id).render(renderer);
    }

    for (int id : temp_table) {
        get_card(id).render(renderer);
    }

    renderer.set_draw_color(sdl::color{0xff, 0x0, 0x0, 0xff});
    for (int id : m_highlights) {
        renderer.draw_rect(get_card_widget(id).get_rect());
    }

    if (m_overlay) {
        sdl::texture *tex = nullptr;
        sdl::rect card_rect;
        if (auto it = m_cards.find(m_overlay); it != m_cards.end()) {
            if (it->second.known) {
                tex = &it->second.texture_front;
                card_rect = it->second.get_rect();
            }
        } else {
            for (auto &[player_id, p] : m_players) {
                if (auto it = std::ranges::find(p.m_characters, m_overlay, &character_card::id); it != p.m_characters.end()) {
                    tex = &it->texture_front;
                    card_rect = it->get_rect();
                    break;
                }
            }
        }
        if (tex) {
            sdl::rect rect = tex->get_rect();
            rect.x = std::clamp(card_rect.x + (card_rect.w - rect.w) / 2, 0, parent->width() - rect.w);
            rect.y = std::clamp(card_rect.y + (card_rect.h - rect.h) / 2, 0, parent->height() -  rect.h);
            tex->render(renderer, rect);
        }
    }

    m_ui.render(renderer);
}

void game_scene::handle_event(const sdl::event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN: {
        sdl::point mouse_pt{event.button.x, event.button.y};
        switch (event.button.button) {
        case SDL_BUTTON_LEFT:
            handle_card_click(mouse_pt);
            break;
        case SDL_BUTTON_RIGHT:
            find_overlay(mouse_pt);
            break;
        }
        break;
    }
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_RIGHT) {
            m_overlay = 0;
        }
        break;
    }
}

void game_scene::handle_card_click(const sdl::point &mouse_pt) {
    auto mouse_in_card = [&](int card_id) {
        return sdl::point_in_rect(mouse_pt, get_card(card_id).get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse, mouse_in_card);
        return (it == pile.rend()) ? 0 : *it;
    };

    sdl::rect main_deck_rect = card_view::texture_back.get_rect();
    sdl::scale_rect(main_deck_rect, card_widget_base::card_width);
    main_deck_rect.x = main_deck.pos.x - main_deck_rect.w / 2;
    main_deck_rect.y = main_deck.pos.y - main_deck_rect.h / 2;

    if (int card_id = find_clicked(temp_table)) {
        on_click_temp_table_card(card_id);
        return;
    }
    if (sdl::point_in_rect(mouse_pt, main_deck_rect)) {
        on_click_main_deck();
        return;
    }
    for (const auto &[player_id, p] : m_players) {
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                on_click_character(player_id, c.id);
                return;
            }
        }
        if (sdl::point_in_rect(mouse_pt, p.m_role.get_rect())) {
            on_click_player(player_id);
            return;
        }
        if (int card_id = find_clicked(p.table)) {
            on_click_table_card(player_id, card_id);
            return;
        }
        if (int card_id = find_clicked(p.hand)) {
            on_click_hand_card(player_id, card_id);
            return;
        }
    }
}

void game_scene::find_overlay(const sdl::point &mouse_pt) {
    auto mouse_in_card = [&](int card_id) {
        return sdl::point_in_rect(mouse_pt, get_card(card_id).get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse, mouse_in_card);
        return (it == pile.rend()) ? 0 : *it;
    };

    sdl::rect main_deck_rect = card_view::texture_back.get_rect();
    sdl::scale_rect(main_deck_rect, card_widget_base::card_width);
    main_deck_rect.x = main_deck.pos.x - main_deck_rect.w / 2;
    main_deck_rect.y = main_deck.pos.y - main_deck_rect.h / 2;

    if (m_overlay = find_clicked(temp_table)) {
        return;
    }
    for (const auto &[player_id, p] : m_players) {
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                m_overlay = c.id;
                return;
            }
        }
        if (!discard_pile.empty() && mouse_in_card(discard_pile.back())) {
            m_overlay = discard_pile.back();
            return;
        }
        if (m_overlay = find_clicked(p.table)) {
            return;
        }
        if (m_overlay = find_clicked(p.hand)) {
            return;
        }
    }
}

constexpr bool is_picking_request(request_type type) {
    constexpr auto lut = []<request_type ... Es>(enums::enum_sequence<Es ...>) {
        return std::array{ picking_request<Es> ... };
    }(enums::make_enum_sequence<request_type>());
    return lut[enums::indexof(type)];
};

constexpr bool is_timer_request(request_type type) {
    constexpr auto lut = []<request_type ... Es>(enums::enum_sequence<Es ...>) {
        return std::array{ timer_request<Es> ... };
    }(enums::make_enum_sequence<request_type>());
    return lut[enums::indexof(type)];
}

void game_scene::on_click_main_deck() {
    if (m_current_request.target_id == m_player_own_id && is_picking_request(m_current_request.type)) {
        add_action<game_action_type::pick_card>(card_pile_type::main_deck);
    } else if (m_playing_id == m_player_own_id) {
        auto &p = get_player(m_playing_id);
        if (auto it = std::ranges::find(p.m_characters, character_type::drawing_forced, &character_card::type); it != p.m_characters.end()) {
            on_click_character(m_playing_id, it->id);
        } else {
            add_action<game_action_type::draw_from_deck>();
        }
    }
}

void game_scene::on_click_temp_table_card(int card_id) {
    if (m_current_request.target_id == m_player_own_id && is_picking_request(m_current_request.type)) {
        add_action<game_action_type::pick_card>(card_pile_type::temp_table, 0, card_id);
    }
}

void game_scene::on_click_table_card(int player_id, int card_id) {
    auto &c = get_card(card_id);
    if (!m_highlights.empty() && card_id == m_highlights.front()) {
        clear_targets();
    } else if (m_current_request.target_id == m_player_own_id && m_current_request.type != request_type::none) {
        if (is_picking_request(m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_table, player_id, card_id);
        } else if (m_play_card_args.card_id == 0) {
            if (!c.inactive) {
                m_play_card_args.card_id = card_id;
                m_highlights.push_back(card_id);
                handle_auto_targets(true);
            }
        } else {
            add_card_target(true, target_card_id{player_id, card_id, false});
        }
    } else if (m_play_card_args.card_id == 0 && c.playable_offturn && !c.inactive) {
        m_play_card_args.card_id = card_id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card_id);
        handle_auto_targets(true);
    } else if (bool(m_play_card_args.flags & play_card_flags::offturn)) {
        add_card_target(true, target_card_id{player_id, card_id, false});
    } else if (m_playing_id == m_player_own_id) {
        if (m_play_card_args.card_id == 0) {
            if (player_id == m_player_own_id) {
                switch (c.color) {
                case card_color_type::green:
                    if (!c.inactive) {
                        m_play_card_args.card_id = card_id;
                        m_highlights.push_back(card_id);
                        handle_auto_targets(false);
                    }
                    break;
                case card_color_type::blue:
                    m_play_card_args.card_id = card_id;
                    m_highlights.push_back(card_id);
                    handle_auto_targets(false);
                    break;
                case card_color_type::brown:
                    throw invalid_action();
                }
            }
        } else {
            add_card_target(false, target_card_id{player_id, card_id, false});
        }
    }
}

void game_scene::on_click_hand_card(int player_id, int card_id) {
    auto &c = get_card(card_id);
    if (!m_highlights.empty() && card_id == m_highlights.front()) {
        clear_targets();
    } else if (m_current_request.target_id == m_player_own_id && m_current_request.type != request_type::none) {
        if (is_picking_request(m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_hand, player_id, card_id);
        } else if (m_play_card_args.card_id == 0) {
            m_play_card_args.card_id = card_id;
            m_highlights.push_back(card_id);
            handle_auto_targets(true);
        } else {
            add_card_target(true, target_card_id{player_id, card_id, true});
        }
    } else if (m_play_card_args.card_id == 0 && c.playable_offturn) {
        m_play_card_args.card_id = card_id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card_id);
        handle_auto_targets(true);
    } else if (bool(m_play_card_args.flags & play_card_flags::offturn)) {
        add_card_target(true, target_card_id{player_id, card_id, true});
    } else if (m_playing_id == m_player_own_id) {
        if (m_play_card_args.card_id == 0) {
            if (player_id == m_player_own_id) {
                switch (c.color) {
                case card_color_type::blue:
                    if (c.equip_targets.empty()
                        || c.equip_targets.front().target == enums::flags_none<target_type>
                        || bool(c.equip_targets.front().target & target_type::self)) {
                        add_action<game_action_type::play_card>(card_id, std::vector{
                            play_card_target{enums::enum_constant<play_card_target_type::target_player>{},
                            std::vector{target_player_id{player_id}}}
                        });
                    } else {
                        m_play_card_args.card_id = card_id;
                        m_play_card_args.flags |= play_card_flags::equipping;
                        m_highlights.push_back(card_id);
                    }
                    break;
                case card_color_type::green:
                    add_action<game_action_type::play_card>(card_id, std::vector{
                        play_card_target{enums::enum_constant<play_card_target_type::target_player>{},
                        std::vector{target_player_id{player_id}}}
                    });
                    break;
                case card_color_type::brown:
                    m_play_card_args.card_id = card_id;
                    m_highlights.push_back(card_id);
                    handle_auto_targets(false);
                    break;
                }
            }
        } else {
            add_card_target(false, target_card_id{player_id, card_id, true});
        }
    }
}

void game_scene::on_click_character(int player_id, int card_id) {
    auto &c = *std::ranges::find(get_player(player_id).m_characters, card_id, &character_card::id);
    if (card_id == m_play_card_args.card_id) {
        clear_targets();
    } else if (m_current_request.target_id == m_player_own_id && m_current_request.type != request_type::none) {
        if (is_picking_request(m_current_request.type)) {
            add_action<game_action_type::pick_card>(card_pile_type::player_character, player_id, card_id);
        } else if (m_play_card_args.card_id == 0 && player_id == m_player_own_id) {
            m_play_card_args.card_id = card_id;
            m_highlights.push_back(card_id);
            handle_auto_targets(true);
        }
    } else if (c.playable_offturn && m_play_card_args.card_id == 0 && player_id == m_player_own_id) {
        m_play_card_args.card_id = card_id;
        m_play_card_args.flags |= play_card_flags::offturn;
        m_highlights.push_back(card_id);
        handle_auto_targets(true);
    } else if (m_playing_id == m_player_own_id) {
        if (m_play_card_args.card_id == 0 && player_id == m_player_own_id) {
            if (c.type != character_type::none) {
                m_play_card_args.card_id = card_id;
                m_highlights.push_back(card_id);
                handle_auto_targets(false);
            }
        }
    }
}

void game_scene::on_click_player(int player_id) {
    if (m_play_card_args.card_id != 0) {
        if (m_current_request.target_id == m_player_own_id && m_current_request.type != request_type::none) {
            if (!is_picking_request(m_current_request.type)) {
                add_player_targets(true, std::vector{target_player_id{player_id}});
            }
        } else if (m_playing_id == m_player_own_id) {
            add_player_targets(false, std::vector{target_player_id{player_id}});
        }
    }
}

std::vector<card_target_data> &game_scene::get_current_card_targets(bool is_response) {
    if (m_virtual) {
        return m_virtual->targets;
    } else {
        auto &c = get_card_widget(m_play_card_args.card_id);
        if (is_response) {
            return c.response_targets;
        } else if (bool(m_play_card_args.flags & play_card_flags::equipping)) {
            return c.equip_targets;
        } else {
            return c.targets;
        }
    }
}

void game_scene::handle_auto_targets(bool is_response) {
    using namespace enums::flag_operators;
    auto do_handle_target = [&](target_type type) {
        switch (type) {
        case enums::flags_none<target_type>:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_none>{});
            return true;
        case target_type::self | target_type::player:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{},
                std::vector{target_player_id{m_player_own_id}});
            return true;
        case target_type::self | target_type::card:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{},
                std::vector{target_card_id{m_player_own_id, m_play_card_args.card_id, false}});
            return true;
        case target_type::attacker | target_type::player:
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{},
                std::vector{target_player_id{m_current_request.origin_id}});
            return true;
        case target_type::everyone | target_type::player: {
            std::vector<target_player_id> ids;
            auto self = m_players.find(m_player_own_id);
            for (auto it = self;;) {
                if (!it->second.dead) {
                    ids.emplace_back(it->first);
                }
                if (++it == m_players.end()) it = m_players.begin();
                if (it == self) break;
            }
            m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_player>{}, ids);
            return true;
        }
        case target_type::everyone | target_type::notself | target_type::player: {
            std::vector<target_player_id> ids;
            auto self = m_players.find(m_player_own_id);
            for (auto it = self;;) {
                if (++it == m_players.end()) it = m_players.begin();
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

void game_scene::add_card_target(bool is_response, const target_card_id &target) {
    if (std::ranges::find(m_highlights, target.card_id) != m_highlights.end()) return;

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
            for (const auto &p : m_players | std::views::values) {
                if (!p.dead) ++ret;
            }
            if (bool(cur_target() & target_type::notself)) {
                --ret;
            }
        }
        return ret;
    };

    if (m_play_card_args.targets.back().enum_index() != play_card_target_type::target_card
        || m_play_card_args.targets.back().get<play_card_target_type::target_card>().size() == num_targets()) {
        m_play_card_args.targets.emplace_back(enums::enum_constant<play_card_target_type::target_card>{});
    }

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
            case target_type::self: return target.player_id == m_player_own_id;
            case target_type::notself: return target.player_id != m_player_own_id;
            case target_type::notsheriff: return get_player(target.player_id).m_role.role != player_role::sheriff;
            case target_type::table: return !target.from_hand;
            case target_type::hand: return target.from_hand;
            case target_type::blue: return get_card(target.card_id).color == card_color_type::blue;
            case target_type::clubs: return get_card(target.card_id).suit == card_suit_type::clubs;
            case target_type::bang: return get_card(target.card_id).name == "Bang!";
            case target_type::missed: return get_card(target.card_id).name == "Mancato!";
            case target_type::bangormissed: {
                const auto &c = get_card(target.card_id);
                return c.name == "Bang!" || c.name == "Mancato!";
            }
            default: return false;
            }
        }))
    {
        m_highlights.push_back(target.card_id);
        auto &l = m_play_card_args.targets.back().get<play_card_target_type::target_card>();
        l.push_back(target);
        if (l.size() == num_targets()) {
            handle_auto_targets(is_response);
        }
    }
}

void game_scene::add_player_targets(bool is_response, const std::vector<target_player_id> &targets) {
    auto &card_targets = get_current_card_targets(is_response);
    auto &cur_target = card_targets[m_play_card_args.targets.size()];
    if (bool(cur_target.target & (target_type::player | target_type::dead))) {
        m_play_card_args.targets.emplace_back(
            enums::enum_constant<play_card_target_type::target_player>{}, targets);
        handle_auto_targets(is_response);
    }
}

void game_scene::clear_targets() {
    m_play_card_args.targets.clear();
    m_play_card_args.card_id = 0;
    m_play_card_args.flags = enums::flags_none<play_card_flags>;
    m_highlights.clear();
    m_virtual.reset();
}

void game_scene::handle_update(const game_update &update) {
    m_pending_updates.push_back(update);
}

void game_scene::pop_update() {
    try {
        if (!m_pending_updates.empty()) {
            const auto update = std::move(m_pending_updates.front());

            m_pending_updates.pop_front();
            enums::visit_indexed([this]<game_update_type E>(enums::enum_constant<E> tag, const auto & ... data) {
                handle_update(tag, data ...);
            }, update);
        }
    } catch (const std::exception &error) {
        std::cerr << "Errore: " << error.what() << '\n';
        parent->disconnect();
    }
}

void game_scene::add_chat_message(const std::string &message) {
    m_ui.add_message(message);
}

void game_scene::show_error(const std::string &message) {
    m_ui.show_error(message);
}

void game_scene::handle_update(enums::enum_constant<game_update_type::game_over>, const game_over_update &args) {
    std::string msg = "Game Over. Winner: ";
    msg += enums::to_string(args.winner_role);
    m_ui.add_message(msg);
}

void game_scene::handle_update(enums::enum_constant<game_update_type::deck_shuffled>) {
    int top_discard = discard_pile.back();
    discard_pile.resize(discard_pile.size() - 1);
    card_move_animation anim;
    for (int id : discard_pile) {
        auto &c = get_card(id);
        c.known = false;
        c.flip_amt = 0.f;
        c.pile = &main_deck;
        main_deck.push_back(id);
        anim.add_move_card(c);
    }
    discard_pile.clear();
    discard_pile.push_back(top_discard);

    m_ui.add_message("Deck shuffled");
    
    m_animations.emplace_back(30, std::move(anim));
}

void game_scene::handle_update(enums::enum_constant<game_update_type::add_cards>, const add_cards_update &args) {
    if (!card_view::texture_back) {
        card_view::make_texture_back();
    }
    for (int id : args.card_ids) {
        auto &c = m_cards[id];
        c.id = id;
        c.pos = main_deck.pos;
        c.pile = &main_deck;
        main_deck.push_back(id);
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::move_card>, const move_card_update &args) {
    auto &c = get_card(args.card_id);
    card_move_animation anim;

    c.pile->erase_card(c.id);
    if (c.pile->width > 0) {
        for (int id : *c.pile) {
            anim.add_move_card(get_card(id));
        }
    }
    
    switch(args.pile) {
    case card_pile_type::player_hand:   c.pile = &get_player(args.player_id).hand; break;
    case card_pile_type::player_table:  c.pile = &get_player(args.player_id).table; break;
    case card_pile_type::main_deck:     c.pile = &main_deck; break;
    case card_pile_type::discard_pile:  c.pile = &discard_pile; break;
    case card_pile_type::temp_table:    c.pile = &temp_table; break;
    }
    c.pile->push_back(c.id);
    if (c.pile->width > 0) {
        for (int id : *c.pile) {
            anim.add_move_card(get_card(id));
        }
    } else {
        anim.add_move_card(c);
    }
    m_animations.emplace_back(20, std::move(anim));
}

void game_scene::handle_update(enums::enum_constant<game_update_type::virtual_card>, const virtual_card_update &args) {
    auto &c = m_virtual.emplace();
    c.id = args.virtual_id;
    c.suit = args.suit;
    c.value = args.value;
    c.color = args.color;
    c.targets = args.targets;

    m_highlights.push_back(args.card_id);
    m_play_card_args.card_id = args.virtual_id;
    handle_auto_targets(false);
}

void game_scene::handle_update(enums::enum_constant<game_update_type::show_card>, const show_card_update &args) {
    auto &c = get_card(args.info.id);

    if (!c.known) {
        static_cast<card_info &>(c) = args.info;
        c.known = true;
        c.suit = args.suit;
        c.value = args.value;
        c.color = args.color;

        c.make_texture_front();

        if (c.pile == &main_deck) {
            std::swap(*std::ranges::find(main_deck, c.id), main_deck.back());
        }
        m_animations.emplace_back(10, card_flip_animation{&c, false});

        if (args.short_pause) {
            m_animations.emplace_back(10, pause_animation{});
        }

    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::hide_card>, const hide_card_update &args) {
    auto &c = get_card(args.card_id);
    if (c.known) {
        c.known = false;
        m_animations.emplace_back(10, card_flip_animation{&c, true});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::tap_card>, const tap_card_update &args) {
    auto &c = get_card(args.card_id);
    if (c.inactive != args.inactive) {
        c.inactive = args.inactive;
        m_animations.emplace_back(10, card_tap_animation{&c, args.inactive});
    } else {
        pop_update();
    }
}

void game_scene::move_player_views() {
    auto own_player = m_players.find(m_player_own_id);
    if (own_player == m_players.end()) return;

    sdl::point pos{parent->width() / 2, parent->height() - 120};
    own_player->second.set_position(pos, true);

    int xradius = (parent->width() - 200) - (parent->width() / 2);
    int yradius = pos.y - (parent->height() / 2);

    auto it = own_player;
    double angle = std::numbers::pi * 1.5f;
    for(;;) {
        if (++it == m_players.end()) it = m_players.begin();
        if (it == own_player) break;
        angle -= std::numbers::pi * 2.f / m_players.size();
        it->second.set_position(sdl::point{
            int(parent->width() / 2 + std::cos(angle) * xradius),
            int(parent->height() / 2 - std::sin(angle) * yradius)
        });
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_add>, const player_user_update &args) {
    if (args.user_id == parent->get_user_own_id()) {
        m_player_own_id = args.player_id;
    }
    auto &p = m_players.try_emplace(args.player_id).first->second;

    p.m_username_text.redraw(parent->get_user_name(args.user_id));

    move_player_views();
    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_hp>, const player_hp_update &args) {
    auto &p = get_player(args.player_id);
    if (p.hp != args.hp && !args.dead) {
        m_animations.emplace_back(20, player_hp_animation{&p, p.hp, args.hp});
    } else {
        pop_update();
    }
    p.dead = args.dead;
    p.hp = args.hp;
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_add_character>, const player_character_update &args) {
    if (!character_card::texture_back) {
        character_card::make_texture_back();
    }
    if (!role_card::texture_back) {
        role_card::make_texture_back();
    }

    auto &p = get_player(args.player_id);
    while (args.index >= p.m_characters.size()) {
        auto &last = p.m_characters.back();
        p.m_characters.emplace_back().pos = sdl::point(last.pos.x + 20, last.pos.y + 20);
    }
    auto &c = *std::next(p.m_characters.begin(), args.index);
    static_cast<card_info &>(c) = args.info;
    c.type = args.type;

    c.make_texture_front();

    if (args.index == 0) {
        p.set_hp_marker_position(p.hp = args.max_hp);
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_remove_character>, const player_remove_character_update &args) {
    auto &p = get_player(args.player_id);
    if (args.index < p.m_characters.size()) {
        p.m_characters.erase(std::next(p.m_characters.begin(), args.index));
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_show_role>, const player_show_role_update &args) {
    auto &p = get_player(args.player_id);
    if (p.m_role.role == args.role) {
        pop_update();
    } else {
        p.m_role.role = args.role;
        p.m_role.make_texture_front();
        if (args.instant) {
            p.m_role.flip_amt = 1.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, card_flip_animation{&p.m_role, false});
        }
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::switch_turn>, const switch_turn_update &args) {
    m_playing_id = args.player_id;
    clear_targets();

    if (m_playing_id == m_player_own_id) {
        m_ui.set_status("Your turn");
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::request_handle>, const request_handle_update &args) {
    m_current_request.type = args.type;
    m_current_request.origin_id = args.origin_id;
    m_current_request.target_id = args.target_id;

    if (is_timer_request(args.type)) {
        std::string message_str = "Timer started: ";
        message_str.append(enums::to_string(args.type));
        m_ui.set_status(message_str);
    } else if (args.target_id == m_player_own_id) {
        std::string message_str = "Respond to: ";
        message_str.append(enums::to_string(args.type));
        m_ui.set_status(message_str);
    } else {
        m_ui.clear_status();
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::status_clear>) {
    m_current_request.type = request_type::none;
    m_current_request.origin_id = 0;

    m_ui.clear_status();

    pop_update();
}