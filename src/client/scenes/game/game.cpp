#include "game.h"
#include "../../manager.h"

#include <iostream>
#include <numbers>

using namespace banggame;
using namespace enums::flag_operators;

game_scene::game_scene(class game_manager *parent)
    : scene_base(parent)
    , m_ui(this)
    , m_target(this)
{
    std::random_device rd;
    rng.seed(rd());
}

static sdl::point cube_pile_offset(auto &rng) {
    return {
        int(rng() % (2 * sizes::cube_pile_size)) - sizes::cube_pile_size,
        int(rng() % (2 * sizes::cube_pile_size)) - sizes::cube_pile_size
    };
}

void game_scene::resize(int width, int height) {
    scene_base::resize(width, height);
    
    m_discard_pile.pos = m_main_deck.pos = sdl::point{width / 2 + sizes::deck_xoffset, height / 2};
    m_discard_pile.pos.x -= sizes::discard_xoffset;
    
    m_selection.pos = sdl::point{width / 2, height / 2 + sizes::selection_yoffset};

    m_shop_hidden.pos = m_shop_discard.pos = m_shop_deck.pos = sdl::point{
        width / 2 + sizes::shop_xoffset - sizes::shop_selection_width - sizes::card_width,
        height / 2};

    m_shop_selection.pos = sdl::point{
        width / 2 + sizes::shop_xoffset - sizes::shop_selection_width / 2,
        height / 2};

    move_player_views();

    if (!m_animations.empty()) {
        m_animations.front().end();
        m_animations.pop_front();
    }

    for (auto &[id, card] : m_cards) {
        card.set_pos(card.pile->get_position(&card));
    }

    for (auto &cube : m_cubes | std::views::values) {
        if (!cube.owner) {
            auto diff = cube_pile_offset(rng);
            cube.pos = sdl::point{
                width / 2 + diff.x + sizes::cube_pile_xoffset,
                height / 2 + diff.y
            };
        }
    }

    m_ui.resize(width, height);
}

template<int N> constexpr auto take_last = std::views::reverse | std::views::take(N) | std::views::reverse;

void game_scene::render(sdl::renderer &renderer) {
    if (m_animations.empty()) {
        pop_update();
    } else {
        auto &anim = m_animations.front();
        anim.tick();
        if (anim.done()) {
            anim.end();
            m_animations.pop_front();
        }
    }

    for (card_view *card : m_main_deck | take_last<2>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_hidden | take_last<1>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_discard | take_last<1>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_deck | take_last<2>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_selection) {
        card->render(renderer);
    }

    for (auto &[player_id, p] : m_players) {
        if (player_id == m_playing_id) {
            p.render_turn_indicator(renderer);
        }
        if (m_current_request.type != request_type::none && m_current_request.target_id == player_id) {
            p.render_request_indicator(renderer);
        }

        p.render(renderer);

        for (card_view *card : p.table) {
            card->render(renderer);
        }
        for (card_view *card : p.hand) {
            card->render(renderer);
        }
    }

    for (card_view *card : m_discard_pile | take_last<2>) {
        card->render(renderer);
    }

    for (card_view *card : m_selection) {
        card->render(renderer);
    }

    if (!m_animations.empty()) {
        m_animations.front().render(renderer);
    }

    for (auto &[_, cube] : m_cubes) {
        cube.render(renderer);
    }

    m_target.render(renderer);
    m_ui.set_button_flags(m_target.get_flags());
    m_ui.render(renderer);

    if (m_overlay && m_overlay->texture_front) {
        sdl::rect rect = m_overlay->texture_front.get_rect();
        sdl::rect card_rect = m_overlay->get_rect();
        rect.x = std::clamp(card_rect.x + (card_rect.w - rect.w) / 2, 0, parent->width() - rect.w);
        rect.y = std::clamp(card_rect.y + (card_rect.h - rect.h) / 2, 0, parent->height() -  rect.h);
        m_overlay->texture_front.render(renderer, rect);
    }
}

void game_scene::handle_event(const sdl::event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN: {
        sdl::point mouse_pt{event.button.x, event.button.y};
        switch (event.button.button) {
        case SDL_BUTTON_LEFT:
            if (m_animations.empty()) {
                handle_card_click(mouse_pt);
            }
            break;
        case SDL_BUTTON_RIGHT:
            find_overlay(mouse_pt);
            break;
        }
        break;
    }
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_RIGHT) {
            m_overlay = nullptr;
        }
        break;
    }
}

void game_scene::handle_card_click(const sdl::point &mouse_pt) {
    auto mouse_in_card = [&](card_widget *card) {
        return sdl::point_in_rect(mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse, mouse_in_card);
        return (it == pile.rend()) ? nullptr : *it;
    };

    sdl::rect main_deck_rect = textures_back::main_deck().get_rect();
    sdl::scale_rect(main_deck_rect, sizes::card_width);
    main_deck_rect.x = m_main_deck.pos.x - main_deck_rect.w / 2;
    main_deck_rect.y = m_main_deck.pos.y - main_deck_rect.h / 2;

    if (card_view *card = find_clicked(m_selection)) {
        m_target.on_click_selection_card(card);
        return;
    }
    if (card_view *card = find_clicked(m_shop_selection)) {
        m_target.on_click_shop_card(card);
        return;
    }
    if (sdl::point_in_rect(mouse_pt, main_deck_rect)) {
        m_target.on_click_main_deck();
        return;
    }
    for (auto &[player_id, p] : m_players) {
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                m_target.on_click_character(player_id, &c);
                return;
            }
        }
        if (sdl::point_in_rect(mouse_pt, p.m_role.get_rect())) {
            m_target.on_click_player(player_id);
            return;
        }
        if (card_view *card = find_clicked(p.table)) {
            m_target.on_click_table_card(player_id, card);
            return;
        }
        if (card_view *card = find_clicked(p.hand)) {
            m_target.on_click_hand_card(player_id, card);
            return;
        }
    }
}

void game_scene::find_overlay(const sdl::point &mouse_pt) {
    auto mouse_in_card = [&](const card_widget *card) {
        return sdl::point_in_rect(mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse,
            [&](const card_view *card) {
                return mouse_in_card(card) && card->known;
            }
        );
        return (it == pile.rend()) ? nullptr : *it;
    };

    sdl::rect main_deck_rect = textures_back::main_deck().get_rect();
    sdl::scale_rect(main_deck_rect, sizes::card_width);
    main_deck_rect.x = m_main_deck.pos.x - main_deck_rect.w / 2;
    main_deck_rect.y = m_main_deck.pos.y - main_deck_rect.h / 2;

    if (m_overlay = find_clicked(m_selection)) {
        return;
    }
    if (m_overlay = find_clicked(m_shop_selection)) {
        return;
    }
    if (!m_discard_pile.empty() && mouse_in_card(m_discard_pile.back())) {
        m_overlay = m_discard_pile.back();
        return;
    }
    for (auto &[player_id, p] : m_players) {
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                m_overlay = &c;
                return;
            }
        }
        if (m_overlay = find_clicked(p.table)) {
            return;
        }
        if (m_overlay = find_clicked(p.hand)) {
            return;
        }
    }
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

void game_scene::handle_update(UPDATE_TAG(game_over), const game_over_update &args) {
    std::string msg = "Game Over. Winner: ";
    msg += enums::to_string(args.winner_role);
    m_ui.add_message(msg);
}

void game_scene::handle_update(UPDATE_TAG(deck_shuffled), const card_pile_type &pile) {
    switch (pile) {
    case card_pile_type::main_deck: {
        card_view *top_discard = m_discard_pile.back();
        m_discard_pile.resize(m_discard_pile.size() - 1);
        card_move_animation anim;
        for (card_view *card : m_discard_pile) {
            card->known = false;
            card->flip_amt = 0.f;
            card->pile = &m_main_deck;
            m_main_deck.push_back(card);
            anim.add_move_card(card);
        }
        m_discard_pile.clear();
        m_discard_pile.push_back(top_discard);
        
        m_animations.emplace_back(30, std::move(anim));
        break;
    }
    case card_pile_type::shop_deck:
        for (card_view *card : m_shop_discard) {
            card->known = false;
            card->flip_amt = 0.f;
            card->pile = &m_shop_deck;
            m_shop_deck.push_back(card);
        }
        m_shop_discard.clear();
        m_animations.emplace_back(20, std::in_place_type<card_flip_animation>, m_shop_deck.back(), true);
        m_animations.emplace_back(20, std::in_place_type<pause_animation>);
        break;
    }
}

void game_scene::handle_update(UPDATE_TAG(add_cards), const add_cards_update &args) {
    for (int id : args.card_ids) {
        auto &c = m_cards[id];
        c.id = id;
        switch (args.pile) {
        case card_pile_type::main_deck:         c.pile = &m_main_deck; c.texture_back = &textures_back::main_deck(); break;
        case card_pile_type::shop_deck:         c.pile = &m_shop_deck; c.texture_back = &textures_back::goldrush(); break;
        case card_pile_type::shop_hidden:       c.pile = &m_shop_hidden; break;
        }
        c.set_pos(c.pile->pos);
        c.pile->push_back(&c);
    }

    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(move_card), const move_card_update &args) {
    auto *card = find_card(args.card_id);
    if (!card) {
        pop_update();
        return;
    }

    card_move_animation anim;

    card->pile->erase_card(card);
    if (card->pile->width > 0) {
        for (card_view *anim_card : *card->pile) {
            anim.add_move_card(anim_card);
        }
    }
    
    card->pile = &[&] () -> card_pile_view& {
        switch(args.pile) {
        case card_pile_type::player_hand:       return find_player(args.player_id)->hand;
        case card_pile_type::player_table:      return find_player(args.player_id)->table;
        case card_pile_type::main_deck:         return m_main_deck;
        case card_pile_type::discard_pile:      return m_discard_pile;
        case card_pile_type::selection:         return m_selection;
        case card_pile_type::shop_deck:         return m_shop_deck;
        case card_pile_type::shop_selection:    return m_shop_selection;
        case card_pile_type::shop_discard:      return m_shop_discard;
        case card_pile_type::shop_hidden:       return m_shop_hidden;
        default: throw std::runtime_error("Pila non valida");
        }
    }();
    card->pile->push_back(card);
    if (card->pile->width > 0) {
        for (card_view *anim_card : *card->pile) {
            anim.add_move_card(anim_card);
        }
    } else {
        anim.add_move_card(card);
    }
    if (bool(args.flags & show_card_flags::no_animation)) {
        anim.end();
        pop_update();
    } else {
        m_animations.emplace_back(20, std::move(anim));
    }
}

void game_scene::handle_update(UPDATE_TAG(add_cubes), const add_cubes_update &args) {
    for (int id : args.cubes) {
        auto &cube = m_cubes.emplace(id, id).first->second;

        auto pos = cube_pile_offset(rng);
        cube.pos = sdl::point{
            parent->width() / 2 + pos.x + sizes::cube_pile_xoffset,
            parent->height() / 2 + pos.y
        };
    }
    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(move_cube), const move_cube_update &args) {
    auto cube_it = m_cubes.find(args.cube_id);
    if (cube_it != m_cubes.end()) {
        auto &cube = cube_it->second;
        if (cube.owner) {
            if (auto it = std::ranges::find(cube.owner->cubes, &cube); it != cube.owner->cubes.end()) {
                cube.owner->cubes.erase(it);
            }
        }
        sdl::point diff = cube_pile_offset(rng);
        if (args.card_id) {
            cube.owner = find_card_widget(args.card_id);
            cube.owner->cubes.push_back(&cube);
        } else {
            cube.owner = nullptr;
            diff.x += parent->width() / 2 + sizes::cube_pile_xoffset;
            diff.y += parent->height() / 2;
        }
        m_animations.emplace_back(8, cube_move_animation(&cube, diff));
    } else {
        pop_update();
    }
}

void game_scene::handle_update(UPDATE_TAG(virtual_card), const virtual_card_update &args) {
    m_target.handle_virtual_card(args);
}

void game_scene::handle_update(UPDATE_TAG(show_card), const show_card_update &args) {
    auto *card = find_card(args.info.id);

    if (card && !card->known) {
        *static_cast<card_info *>(card) = args.info;
        card->known = true;
        card->suit = args.suit;
        card->value = args.value;
        card->color = args.color;

        card->make_texture_front();

        if (card->pile == &m_main_deck) {
            std::swap(*std::ranges::find(m_main_deck, card), m_main_deck.back());
        } else if (card->pile == &m_shop_deck) {
            std::swap(*std::ranges::find(m_shop_deck, card), m_shop_deck.back());
        }
        if (bool(args.flags & show_card_flags::no_animation)) {
            card->flip_amt = 1.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, std::in_place_type<card_flip_animation>, card, false);

            if (bool(args.flags & show_card_flags::short_pause)) {
                m_animations.emplace_back(20, std::in_place_type<pause_animation>);
            }
        }

    } else {
        pop_update();
    }
}

void game_scene::handle_update(UPDATE_TAG(hide_card), const hide_card_update &args) {
    auto *card = find_card(args.card_id);

    if (card && card->known) {
        card->known = false;
        if (bool(args.flags & show_card_flags::no_animation)) {
            card->flip_amt = 0.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, std::in_place_type<card_flip_animation>, card, true);

            if (bool(args.flags & show_card_flags::short_pause)) {
                m_animations.emplace_back(20, std::in_place_type<pause_animation>);
            }
        }
    } else {
        pop_update();
    }
}

void game_scene::handle_update(UPDATE_TAG(tap_card), const tap_card_update &args) {
    auto *card = find_card(args.card_id);
    if (card->inactive != args.inactive) {
        card->inactive = args.inactive;
        m_animations.emplace_back(10, std::in_place_type<card_tap_animation>, card, args.inactive);
    } else {
        pop_update();
    }
}

void game_scene::move_player_views() {
    auto own_player = m_player_own_id ? m_players.find(m_player_own_id) : m_players.begin();
    if (own_player == m_players.end()) return;

    int xradius = (parent->width() / 2) - sizes::player_ellipse_x_distance;
    int yradius = (parent->height() / 2) - sizes::player_ellipse_y_distance;

    double angle = 0.f;
    for(auto it = own_player;;) {
        it->second.set_position(sdl::point{
            int(parent->width() / 2 - std::sin(angle) * xradius),
            int(parent->height() / 2 + std::cos(angle) * yradius)
        }, it == own_player);
        
        angle += std::numbers::pi * 2.f / m_players.size();
        if (++it == m_players.end()) it = m_players.begin();
        if (it == own_player) break;
    }
}

void game_scene::handle_update(UPDATE_TAG(player_add), const player_user_update &args) {
    if (args.user_id == parent->get_user_own_id()) {
        m_player_own_id = args.player_id;
    }
    auto &p = m_players.try_emplace(args.player_id).first->second;

    p.m_username_text.redraw(parent->get_user_name(args.user_id));

    move_player_views();
    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(player_hp), const player_hp_update &args) {
    auto *player = find_player(args.player_id);
    if (player->hp != args.hp && !args.dead) {
        m_animations.emplace_back(20, std::in_place_type<player_hp_animation>, player, player->hp, args.hp);
    } else {
        pop_update();
    }
    player->dead = args.dead;
    player->hp = args.hp;
}

void game_scene::handle_update(UPDATE_TAG(player_gold), const player_gold_update &args) {
    find_player(args.player_id)->set_gold(args.gold);
    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(player_add_character), const player_character_update &args) {
    auto &p = *find_player(args.player_id);
    p.m_role.texture_back = &textures_back::role();

    while (args.index >= p.m_characters.size()) {
        auto &last_pos = p.m_characters.back().get_pos();
        p.m_characters.emplace_back().set_pos(sdl::point(last_pos.x + 20, last_pos.y + 20));
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

void game_scene::handle_update(UPDATE_TAG(player_remove_character), const player_remove_character_update &args) {
    auto &p = *find_player(args.player_id);
    if (args.index < p.m_characters.size()) {
        p.m_characters.erase(std::next(p.m_characters.begin(), args.index));
    }

    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(player_show_role), const player_show_role_update &args) {
    auto &p = *find_player(args.player_id);
    if (p.m_role.role == args.role) {
        pop_update();
    } else {
        p.m_role.role = args.role;
        p.m_role.make_texture_front();
        if (args.instant) {
            if (args.role == player_role::sheriff) {
                p.set_hp_marker_position(++p.hp);
            }
            p.m_role.flip_amt = 1.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, std::in_place_type<card_flip_animation>, &p.m_role, false);
        }
    }
}

void game_scene::handle_update(UPDATE_TAG(switch_turn), const switch_turn_update &args) {
    m_playing_id = args.player_id;
    m_target.clear_targets();

    if (m_playing_id == m_player_own_id) {
        m_ui.set_status("Your turn");
    }

    pop_update();
}

void game_scene::handle_update(UPDATE_TAG(request_handle), const request_handle_update &args) {
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

void game_scene::handle_update(UPDATE_TAG(status_clear)) {
    m_current_request.type = request_type::none;
    m_current_request.origin_id = 0;

    m_ui.clear_status();

    pop_update();
}