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

    m_ui.enable_restart(false);
}

void game_scene::init(const game_started_args &args) {
    m_expansions = args.expansions;

    if (!bool(args.expansions & card_expansion_type::goldrush)) {
        m_ui.disable_goldrush();
    }
}

static sdl::point cube_pile_offset(auto &rng) {
    std::normal_distribution<float> dist{0, sizes::cube_pile_size};
    return {int(dist(rng)), int(dist(rng))};
}

void game_scene::resize(int width, int height) {
    scene_base::resize(width, height);
    
    m_discard_pile.pos = m_main_deck.pos = sdl::point{width / 2 + sizes::deck_xoffset, height / 2};
    m_discard_pile.pos.x -= sizes::discard_xoffset;
    
    m_selection.pos = sdl::point{width / 2, height / 2 + sizes::selection_yoffset};

    m_shop_discard.pos = m_shop_deck.pos = sdl::point{
        width / 2 + sizes::shop_xoffset - sizes::shop_selection_width - sizes::card_width,
        height / 2};

    m_shop_selection.pos = sdl::point{
        width / 2 + sizes::shop_xoffset - sizes::shop_selection_width / 2,
        height / 2};

    m_scenario_card.pos = sdl::point{width / 2 + sizes::deck_xoffset + sizes::card_width + sizes::card_xoffset, height / 2};

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

    for (card_view *card : m_shop_discard | take_last<1>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_deck | take_last<2>) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_selection) {
        card->render(renderer);
    }

    for (card_view *card : m_scenario_deck | take_last<1>) {
        card->render(renderer);
    }

    for (card_view *card : m_scenario_card | take_last<2>) {
        card->render(renderer);
    }

    for (auto &cube : m_cubes | std::views::values) {
        if (cube.owner == nullptr) {
            cube.render(renderer);
        }
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
        case SDL_BUTTON_MIDDLE:
            find_overlay(mouse_pt);
            break;
        case SDL_BUTTON_RIGHT:
            m_target.clear_targets();
            break;
        }
        break;
    }
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_MIDDLE) {
            m_overlay = nullptr;
        }
        break;
    case SDL_KEYDOWN:
        if (sdl::event_handler::is_focused(nullptr)) {
            switch(event.key.keysym.sym) {
            case SDLK_a: m_target.on_click_pass_turn(); break;
            case SDLK_s: m_target.on_click_resolve(); break;
            case SDLK_d: m_target.on_click_sell_beer(); break;
            case SDLK_f: m_target.on_click_discard_black(); break;
            default: break;
            }
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

    sdl::rect main_deck_rect = card_textures::main_deck().get_rect();
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
    if (!m_scenario_card.empty() && mouse_in_card(m_scenario_card.back())) {
        m_target.on_click_scenario_card(m_scenario_card.back());
        return;
    }
    for (auto &p : m_players | std::views::values) {
        if (sdl::point_in_rect(mouse_pt, p.m_bounding_rect)) {
            if (m_target.on_click_player(&p)) {
                return;
            }
        }
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                m_target.on_click_character(&p, &c);
                return;
            }
        }
        if (card_view *card = find_clicked(p.table)) {
            m_target.on_click_table_card(&p, card);
            return;
        }
        if (card_view *card = find_clicked(p.hand)) {
            m_target.on_click_hand_card(&p, card);
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

    sdl::rect main_deck_rect = card_textures::main_deck().get_rect();
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
    if (!m_scenario_deck.empty() && mouse_in_card(m_scenario_deck.back())) {
        m_overlay = m_scenario_deck.back();
        return;
    }
    if (!m_scenario_card.empty() && mouse_in_card(m_scenario_card.back())) {
        m_overlay = m_scenario_card.back();
        return;
    }
    for (auto &p : m_players | std::views::values) {
        for (auto &c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(mouse_pt, c.get_rect())) {
                m_overlay = &c;
                return;
            }
        }
        if (mouse_in_card(&p.m_role)) {
            m_overlay = &p.m_role;
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

void game_scene::handle_game_update(const game_update &update) {
    m_pending_updates.push_back(update);
}

void game_scene::add_user(int id, const user_info &args) {
    m_ui.add_message(_("GAME_USER_CONNECTED", args.name));
}

void game_scene::remove_user(int id) {
    auto it = std::ranges::find(m_players, id, [](const auto &pair) { return pair.second.user_id; });
    if (it != m_players.end()) {
        it->second.user_id = 0;
        it->second.set_username(_("USERNAME_DISCONNECTED"));
        it->second.set_profile_image(nullptr);
    }
    user_info *info = parent->get_user_info(id);
    if (info) {
        m_ui.add_message(_("GAME_USER_DISCONNECTED", info->name));
    }
}

void game_scene::pop_update() {
    try {
        if (!m_pending_updates.empty()) {
            const auto update = std::move(m_pending_updates.front());

            m_pending_updates.pop_front();
            enums::visit_indexed([this]<game_update_type E>(enums::enum_constant<E> tag, const auto & ... data) {
                handle_game_update(tag, data ...);
            }, update);
        }
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        parent->disconnect();
    }
}

void game_scene::add_chat_message(const std::string &message) {
    m_ui.add_message(message);
}

void game_scene::show_error(const std::string &message) {
    m_ui.show_error(message);
}

void game_scene::handle_game_update(UPDATE_TAG(game_over), const game_over_update &args) {
    m_ui.add_message(_("STATUS_GAME_OVER", _(args.winner_role)));
    
    if (parent->get_user_own_id() == parent->get_lobby_owner_id()) {
        m_ui.enable_restart(true);
    }
}

std::string game_scene::get_card_name(card_widget *card) {
    if (!card || !card->known) return _("UNKNOWN_CARD");
    if (card->value != card_value_type::none && card->suit != card_suit_type::none) {
        return intl::format("{} ({}{})", card->name, enums::get_data(card->value), enums::get_data(card->suit).symbol);
    } else {
        return card->name;
    }
}

void game_scene::handle_game_update(UPDATE_TAG(game_log), const game_log_update &args) {
    std::cout << _(args.message,
        args.origin_card_id ? get_card_name(find_card_widget(args.origin_card_id)) : "-",
        args.origin_id ? find_player(args.origin_id)->m_username_text.get_value() : "-",
        args.target_id ? find_player(args.target_id)->m_username_text.get_value() : "-",
        args.target_card_id ? get_card_name(find_card_widget(args.target_card_id)) : "-",
        args.custom_value
    ) << '\n';

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(deck_shuffled), const card_pile_type &pile) {
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

void game_scene::handle_game_update(UPDATE_TAG(add_cards), const add_cards_update &args) {
    for (int id : args.card_ids) {
        auto &c = m_cards[id];
        c.id = id;
        switch (args.pile) {
        case card_pile_type::main_deck:         c.pile = &m_main_deck; c.texture_back = &card_textures::main_deck(); break;
        case card_pile_type::shop_deck:         c.pile = &m_shop_deck; c.texture_back = &card_textures::goldrush(); break;
        case card_pile_type::scenario_deck:     c.pile = &m_scenario_deck; break;
        case card_pile_type::hidden_deck:       c.pile = &m_hidden_deck; break;
        default: throw std::runtime_error("Invalid pile");
        }
        c.set_pos(c.pile->pos);
        c.pile->push_back(&c);
    }

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(move_card), const move_card_update &args) {
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
        case card_pile_type::hidden_deck:       return m_hidden_deck;
        case card_pile_type::scenario_card:     return m_scenario_card;
        default: throw std::runtime_error("Invalid pile");
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

        if (bool(args.flags & show_card_flags::short_pause)) {
            m_animations.emplace_back(20, std::in_place_type<pause_animation>);
        } else {
            pop_update();
        }
    } else {
        m_animations.emplace_back(20, std::move(anim));
    }
}

void game_scene::handle_game_update(UPDATE_TAG(add_cubes), const add_cubes_update &args) {
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

void game_scene::handle_game_update(UPDATE_TAG(move_cube), const move_cube_update &args) {
    auto [cube_it, inserted] = m_cubes.try_emplace(args.cube_id, args.cube_id);
    auto &cube = cube_it->second;
    if (cube.owner) {
        if (auto it = std::ranges::find(cube.owner->cubes, &cube); it != cube.owner->cubes.end()) {
            cube.owner->cubes.erase(it);
        }
    }
    sdl::point diff;
    if (args.card_id) {
        cube.owner = find_card_widget(args.card_id);
        diff.x = sizes::cube_xdiff;
        diff.y = sizes::cube_ydiff + sizes::cube_yoff * cube.owner->cubes.size();
        cube.owner->cubes.push_back(&cube);
    } else {
        cube.owner = nullptr;
        diff = cube_pile_offset(rng);
        diff.x += parent->width() / 2 + sizes::cube_pile_xoffset;
        diff.y += parent->height() / 2;
    }
    cube_move_animation anim(&cube, diff);
    if (inserted) {
        anim.end();
        pop_update();
    } else {
        m_animations.emplace_back(8, std::move(anim));
    }
}

void game_scene::handle_game_update(UPDATE_TAG(virtual_card), const virtual_card_update &args) {
    m_target.handle_virtual_card(args);
}

void game_scene::handle_game_update(UPDATE_TAG(show_card), const show_card_update &args) {
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

void game_scene::handle_game_update(UPDATE_TAG(hide_card), const hide_card_update &args) {
    auto *card = find_card(args.card_id);

    if (card && card->known && args.ignore_player_id != m_player_own_id) {
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

void game_scene::handle_game_update(UPDATE_TAG(tap_card), const tap_card_update &args) {
    auto *card = find_card(args.card_id);
    if (card->inactive != args.inactive) {
        card->inactive = args.inactive;
        if (args.instant) {
            card->rotation = card->inactive ? 90.f : 0.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, std::in_place_type<card_tap_animation>, card, args.inactive);
        }
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

    if (auto it = std::ranges::find(m_players, m_players.size() < 4 ? player_role::deputy : player_role::sheriff,
        [](const auto &pair) { return pair.second.m_role.role; }); it != m_players.end()) {
        auto player_rect = it->second.m_bounding_rect;
        m_scenario_deck.pos.x = player_rect.x + player_rect.w + sizes::scenario_deck_xoff;
        m_scenario_deck.pos.y = player_rect.y + player_rect.h / 2;

        for (const auto &c : m_scenario_deck) {
            c->set_pos(m_scenario_deck.pos);
        }
    }
}

void game_scene::handle_game_update(UPDATE_TAG(player_add), const player_user_update &args) {
    if (args.user_id == parent->get_user_own_id()) {
        m_player_own_id = args.player_id;
    }
    auto &p = m_players.try_emplace(args.player_id, args.player_id).first->second;

    p.user_id = args.user_id;
    if (user_info *info = parent->get_user_info(args.user_id)) {
        p.set_username(info->name);
        p.set_profile_image(&info->profile_image);
    } else {
        p.set_username(_("USERNAME_DISCONNECTED"));
        p.set_profile_image(nullptr);
    }

    move_player_views();
    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(player_hp), const player_hp_update &args) {
    auto *player = find_player(args.player_id);
    int prev_hp = player->hp;
    player->dead = args.dead;
    player->hp = args.hp;
    if (args.instant) {
        player->set_hp_marker_position(args.hp);
        pop_update();
    } else if (prev_hp != args.hp && !args.dead) {
        m_animations.emplace_back(20, std::in_place_type<player_hp_animation>, player, prev_hp, args.hp);
    } else {
        pop_update();
    }
}

void game_scene::handle_game_update(UPDATE_TAG(player_gold), const player_gold_update &args) {
    find_player(args.player_id)->set_gold(args.gold);
    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(player_add_character), const player_character_update &args) {
    auto &p = *find_player(args.player_id);
    p.m_role.texture_back = &card_textures::role();

    while (args.index >= p.m_characters.size()) {
        auto &last_pos = p.m_characters.back().get_pos();
        p.m_characters.emplace_back().set_pos(sdl::point(last_pos.x + 20, last_pos.y + 20));
    }
    auto &c = *std::next(p.m_characters.begin(), args.index);

    static_cast<card_info &>(c) = args.info;
    c.known = true;
    c.make_texture_front();

    if (args.index == 0) {
        p.set_hp_marker_position(p.hp = args.max_hp);
    }

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(player_remove_character), const player_remove_character_update &args) {
    auto &p = *find_player(args.player_id);
    if (args.index < p.m_characters.size()) {
        p.m_characters.erase(std::next(p.m_characters.begin(), args.index));
    }

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(player_show_role), const player_show_role_update &args) {
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

    move_player_views();
}

void game_scene::handle_game_update(UPDATE_TAG(switch_turn), const switch_turn_update &args) {
    m_playing_id = args.player_id;
    m_target.clear_targets();

    if (m_playing_id == m_player_own_id && m_current_request.type == request_type::none) {
        m_ui.set_status(_("STATUS_YOUR_TURN"));
    }

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(request_handle), const request_view &args) {
    m_current_request = args;

    using namespace enums::stream_operators;
    
    constexpr auto timer_lut = []<request_type ... Es>(enums::enum_sequence<Es ...>) {
        return std::array{ timer_request<Es> ... };
    }(enums::make_enum_sequence<request_type>());

    if (timer_lut[enums::indexof(args.type)] || args.target_id == m_player_own_id) {
        auto *origin_card = find_card_widget(args.origin_card_id);
        auto *target_card = find_card_widget(args.target_card_id);
        auto *target_player = find_player(args.target_id);
        m_ui.set_status(_(args.type,
            origin_card ? get_card_name(origin_card) : "-",
            target_card ? target_card->pile == &target_player->hand
                ? _("STATUS_CARD_FROM_HAND")
                : get_card_name(target_card)
            : "-",
            target_player->m_username_text.get_value()));
    } else {
        m_ui.clear_status();
    }

    pop_update();
}

void game_scene::handle_game_update(UPDATE_TAG(status_clear)) {
    m_current_request.type = request_type::none;
    m_current_request.origin_id = 0;

    m_ui.clear_status();

    pop_update();
}