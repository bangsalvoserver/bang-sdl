#include "game.h"
#include "../manager.h"
#include "../media_pak.h"
#include "utils/utils.h"

#include <iostream>
#include <numbers>
#include <ranges>

using namespace banggame;
using namespace enums::flag_operators;

game_scene::game_scene(class game_manager *parent, const game_started_args &args)
    : scene_base(parent)
    , m_card_textures(parent->get_base_path())
    , m_ui(this)
    , m_target(this)
{
    std::random_device rd;
    rng.seed(rd());

    m_ui.enable_restart(false);
    
    m_expansions = args.expansions;
}

static sdl::point cube_pile_offset(auto &rng) {
    std::uniform_int_distribution<int> dist{
        -options::cube_pile_size / 2,
        options::cube_pile_size / 2
    };
    return {dist(rng), dist(rng)};
}

void game_scene::update_main_deck_count() {
    m_main_deck_count.redraw(std::to_string(m_main_deck.size()));
    sdl::rect rect = m_main_deck_count.get_rect();
    rect.x = m_main_deck.get_pos().x - rect.w / 2;
    rect.y = m_main_deck.get_pos().y - rect.h / 2;
    m_main_deck_count.set_rect(rect);
}

void game_scene::resize(int width, int height) {
    scene_base::resize(width, height);

    m_main_deck.set_pos(sdl::point{
        width / 2 + options::deck_xoffset,
        height / 2});

    update_main_deck_count();

    m_discard_pile.set_pos(sdl::point{
        m_main_deck.get_pos().x - options::discard_xoffset,
        m_main_deck.get_pos().y});
    
    m_selection.set_pos(sdl::point{
        width / 2,
        height / 2 + options::selection_yoffset});

    m_shop_deck.set_pos(sdl::point{
        width / 2 + options::shop_xoffset - options::shop_selection_width - options::card_width,
        height / 2});

    m_shop_discard.set_pos(m_shop_deck.get_pos());

    m_shop_selection.set_pos(sdl::point{
        width / 2 + options::shop_xoffset - options::shop_selection_width / 2,
        height / 2});
    
    m_shop_choice.set_pos(sdl::point{
        m_shop_selection.get_pos().x,
        m_shop_selection.get_pos().y + options::shop_choice_offset});

    m_scenario_card.set_pos(sdl::point{
        width / 2 + options::deck_xoffset + options::card_width + options::card_xoffset,
        height / 2});

    move_player_views();

    for (auto &cube : m_cubes | std::views::values) {
        if (!cube.owner) {
            auto diff = cube_pile_offset(rng);
            cube.pos = sdl::point{
                width / 2 + diff.x + options::cube_pile_xoffset,
                height / 2 + diff.y
            };
        }
    }

    m_ui.resize(width, height);
}

template<int N> constexpr auto take_last = std::views::reverse | std::views::take(N) | std::views::reverse;

void game_scene::render(sdl::renderer &renderer) {
    if (m_mouse_motion_timer >= options::card_overlay_timer) {
        if (!m_overlay) {
            find_overlay();
        }
    } else {
        if (!m_middle_click) {
            m_overlay = nullptr;
        }
        ++m_mouse_motion_timer;
    }

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

    m_target.set_border_colors();

    if (!m_main_deck.empty() && m_main_deck.border_color) {
        sdl::rect rect = m_main_deck.back()->get_rect();
        card_textures::get().card_border.render_colored(renderer, sdl::rect{
            rect.x - options::default_border_thickness,
            rect.y - options::default_border_thickness,
            rect.w + options::default_border_thickness * 2,
            rect.h + options::default_border_thickness * 2
        }, sdl::rgba(m_main_deck.border_color));
    }
    for (card_view *card : m_main_deck | take_last<2>) {
        card->render(renderer);
    }

    m_main_deck_count.render(renderer);

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
        p.render(renderer);

        for (card_view *card : p.table) {
            card->render(renderer);
        }
        for (card_view *card : p.hand) {
            card->render(renderer);
        }

        int x = p.m_bounding_rect.x + p.m_bounding_rect.w - 5;

        auto render_icon = [&](const sdl::texture &texture, sdl::color color = sdl::rgba(0xffffffff)) {
            sdl::rect rect = texture.get_rect();
            rect.x = x - rect.w;
            rect.y = p.m_bounding_rect.y + 5;
            texture.render_colored(renderer, rect, color);
        };

        if (m_winner_role == player_role::unknown) {
            if (player_id == m_playing_id) {
                render_icon(media_pak::get().icon_turn, sdl::rgba(options::turn_indicator_rgba));
                x -= 32;
            }
            if (m_request_target_id == player_id) {
                render_icon(media_pak::get().icon_target, sdl::rgba(options::request_target_indicator_rgba));
                x -= 32;
            }
            if (m_request_origin_id == player_id) {
                render_icon(media_pak::get().icon_origin, sdl::rgba(options::request_origin_indicator_rgba));
            }
        } else if (p.m_role.role == m_winner_role
            || (p.m_role.role == player_role::deputy && m_winner_role == player_role::sheriff)
            || (p.m_role.role == player_role::sheriff && m_winner_role == player_role::deputy)) {
            render_icon(media_pak::get().icon_winner, sdl::rgba(options::winner_indicator_rgba));
        }
    }

    if (!m_discard_pile.empty() && m_discard_pile.border_color) {
        sdl::rect rect = m_discard_pile.back()->get_rect();
        card_textures::get().card_border.render_colored(renderer, sdl::rect{
            rect.x - options::default_border_thickness,
            rect.y - options::default_border_thickness,
            rect.w + options::default_border_thickness * 2,
            rect.h + options::default_border_thickness * 2
        }, sdl::rgba(m_discard_pile.border_color));
    }
    for (card_view *card : m_discard_pile | take_last<2>) {
        card->render(renderer);
    }

    for (card_view *card : m_selection) {
        card->render(renderer);
    }

    for (card_view *card : m_shop_choice) {
        card->render(renderer);
    }

    if (!m_animations.empty()) {
        m_animations.front().render(renderer);
    }

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
    case SDL_MOUSEBUTTONDOWN:
        m_mouse_pt = {event.button.x, event.button.y};
        switch (event.button.button) {
        case SDL_BUTTON_LEFT:
            if (m_target.is_card_clickable()) {
                handle_card_click();
            }
            break;
        case SDL_BUTTON_RIGHT:
            if (!m_target.waiting_confirm()) {
                m_target.clear_targets();
            }
            break;
        case SDL_BUTTON_MIDDLE:
            m_middle_click = true;
            find_overlay();
            break;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        m_mouse_pt = {event.button.x, event.button.y};
        switch (event.button.button) {
        case SDL_BUTTON_MIDDLE:
            m_middle_click = false;
            break;
        }
        break;
    case SDL_MOUSEMOTION:
        m_mouse_motion_timer = 0;
        m_mouse_pt = {event.motion.x, event.motion.y};
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_RETURN) {
            parent->enable_chat();
        }
        break;
    }
}

void game_scene::handle_card_click() {
    auto mouse_in_card = [&](card_view *card) {
        return sdl::point_in_rect(m_mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse, mouse_in_card);
        return (it == pile.rend()) ? nullptr : *it;
    };

    if (card_view *card = find_clicked(m_selection)) {
        m_target.on_click_selection_card(card);
        return;
    }
    if (card_view *card = find_clicked(m_shop_choice.empty() ? m_shop_selection : m_shop_choice)) {
        m_target.on_click_shop_card(card);
        return;
    }
    if (sdl::point_in_rect(m_mouse_pt, m_main_deck.back()->get_rect())) {
        m_target.on_click_main_deck();
        return;
    }
    if (!m_discard_pile.empty() && sdl::point_in_rect(m_mouse_pt, m_discard_pile.back()->get_rect())) {
        m_target.on_click_discard_pile();
        return;
    }
    if (!m_scenario_card.empty() && mouse_in_card(m_scenario_card.back())) {
        m_target.on_click_scenario_card(m_scenario_card.back());
        return;
    }
    for (auto &p : m_players | std::views::values) {
        if (sdl::point_in_rect(m_mouse_pt, p.m_bounding_rect)) {
            if (m_target.on_click_player(&p)) {
                return;
            }
        }
        for (card_view *c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(m_mouse_pt, c->get_rect())) {
                m_target.on_click_character(&p, c);
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

void game_scene::find_overlay() {
    auto mouse_in_card = [&](const card_view *card) {
        return sdl::point_in_rect(m_mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const card_pile_view &pile) {
        auto it = std::ranges::find_if(pile | std::views::reverse,
            [&](const card_view *card) {
                return mouse_in_card(card) && card->known;
            }
        );
        return (it == pile.rend()) ? nullptr : *it;
    };

    if (m_overlay = find_clicked(m_shop_choice)) {
        return;
    }
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
        for (card_view *c : p.m_characters | std::views::reverse) {
            if (sdl::point_in_rect(m_mouse_pt, c->get_rect())) {
                m_overlay = c;
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
    parent->add_chat_message(message_type::server_log, _("GAME_USER_CONNECTED", args.name));
}

void game_scene::remove_user(int id) {
    auto it = std::ranges::find(m_players, id, [](const auto &pair) { return pair.second.user_id; });
    if (it != m_players.end()) {
        it->second.user_id = 0;
        it->second.set_username(_("USERNAME_DISCONNECTED"));
        it->second.m_propic.set_texture(media_pak::get().icon_disconnected);
    }
    user_info *info = parent->get_user_info(id);
    if (info) {
        parent->add_chat_message(message_type::server_log, _("GAME_USER_DISCONNECTED", info->name));
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
        parent->disconnect(std::string("Error: ") + error.what());
    }
}

void game_scene::HANDLE_UPDATE(game_over, const game_over_update &args) {
    m_ui.set_status(_("STATUS_GAME_OVER"));
    m_winner_role = args.winner_role;
    
    if (parent->get_user_own_id() == parent->get_lobby_owner_id()) {
        m_ui.enable_restart(true);
    }
}

std::string game_scene::evaluate_format_string(const game_formatted_string &str) {
    return intl::format(str.localized ? intl::translate(str.format_str) : str.format_str,
        str.format_args | std::views::transform([&](const game_format_arg &arg) {
        return std::visit(util::overloaded{
            [](int value) { return std::to_string(value); },
            [](const std::string &value) { return value; },
            [&](card_format_id value) {
                player_view *owner = value.player_id ? find_player(value.player_id) : nullptr;
                card_view *card = value.card_id ? find_card(value.card_id) : nullptr;
                if (card) {
                    if (owner && card->pile == &owner->hand) {
                        return _("STATUS_CARD_FROM_HAND");
                    } else if (!card->known) {
                        return _("STATUS_UNKNOWN_CARD");
                    } else if (card->value != card_value_type::none && card->suit != card_suit_type::none) {
                        return intl::format("{} ({}{})", card->name, enums::get_data(card->value), enums::get_data(card->suit).symbol);
                    } else {
                        return card->name;
                    }
                } else {
                    return _("STATUS_UNKNOWN_CARD");
                }
            },
            [&](player_format_id value) {
                return value.player_id
                    ? find_player(value.player_id)->m_username_text.get_value()
                    : _("STATUS_UNKNOWN_PLAYER");
            }
        }, arg);
    }));
}

void game_scene::HANDLE_UPDATE(game_error, const game_formatted_string &args) {
    parent->add_chat_message(message_type::error, evaluate_format_string(args));
    pop_update();
}

void game_scene::HANDLE_UPDATE(game_log, const game_formatted_string &args) {
    m_ui.add_game_log(evaluate_format_string(args));
    pop_update();
}

void game_scene::HANDLE_UPDATE(deck_shuffled, const card_pile_type &pile) {
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
        update_main_deck_count();
        
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

void game_scene::HANDLE_UPDATE(add_cards, const add_cards_update &args) {
    auto add_cards_to = [&](card_pile_view &pile, const sdl::texture *texture = nullptr) {
        for (int id : args.card_ids) {
            auto &c = m_cards[id];
            c.id = id;
            c.pile = &pile;
            c.texture_back = texture;
            pile.push_back(&c);
            c.set_pos(pile.get_position_of(&c));
        }
    };

    switch (args.pile) {
    case card_pile_type::main_deck:
        add_cards_to(m_main_deck, &card_textures::get().backface_maindeck);
        update_main_deck_count();
        break;
    case card_pile_type::player_character:
        add_cards_to(find_player(args.player_id)->m_characters, &card_textures::get().backface_character);
        break;
    case card_pile_type::player_backup:
        add_cards_to(find_player(args.player_id)->m_backup_characters, &card_textures::get().backface_character);
        break;
    case card_pile_type::shop_deck:         add_cards_to(m_shop_deck, &card_textures::get().backface_goldrush); break;
    case card_pile_type::scenario_deck:     add_cards_to(m_scenario_deck); break;
    case card_pile_type::hidden_deck:       add_cards_to(m_hidden_deck); break;
    case card_pile_type::specials:          add_cards_to(m_specials); break;
    default: throw std::runtime_error("Invalid pile");
    }

    pop_update();
}

void game_scene::HANDLE_UPDATE(remove_cards, const remove_cards_update &args) {
    for (int id : args.card_ids) {
        auto *c = find_card(id);
        if (c && c->pile) {
            c->pile->erase_card(c);
        }
        m_cards.erase(id);
    }

    pop_update();
}

void game_scene::HANDLE_UPDATE(move_card, const move_card_update &args) {
    card_view *card = find_card(args.card_id);
    if (!card) {
        pop_update();
        return;
    }

    auto *new_pile = &[&] () -> card_pile_view& {
        switch(args.pile) {
        case card_pile_type::player_hand:       return find_player(args.player_id)->hand;
        case card_pile_type::player_table:      return find_player(args.player_id)->table;
        case card_pile_type::player_character:  return find_player(args.player_id)->m_characters;
        case card_pile_type::player_backup:     return find_player(args.player_id)->m_backup_characters;
        case card_pile_type::main_deck:         return m_main_deck;
        case card_pile_type::discard_pile:      return m_discard_pile;
        case card_pile_type::selection:         return m_selection;
        case card_pile_type::shop_deck:         return m_shop_deck;
        case card_pile_type::shop_selection:    return m_shop_selection;
        case card_pile_type::shop_discard:      return m_shop_discard;
        case card_pile_type::hidden_deck:       return m_hidden_deck;
        case card_pile_type::scenario_card:     return m_scenario_card;
        case card_pile_type::specials:          return m_specials;
        default: throw std::runtime_error("Invalid pile");
        }
    }();

    if (card->pile == new_pile) {
        pop_update();
        return;
    }

    card_move_animation anim;

    card->pile->erase_card(card);
    if (card->pile->width() > 0) {
        for (card_view *anim_card : *card->pile) {
            anim.add_move_card(anim_card);
        }
    }

    new_pile->push_back(card);
    if (new_pile == &m_main_deck || card->pile == &m_main_deck) {
        update_main_deck_count();
    }
    
    card->pile = new_pile;
    if (card->pile->width() > 0) {
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
        if (bool(args.flags & show_card_flags::pause_before_move)) {
            m_animations.emplace_back(20, std::in_place_type<pause_animation>, card);
        }
        
        m_animations.emplace_back(20, std::move(anim));
    }
}

void game_scene::HANDLE_UPDATE(add_cubes, const add_cubes_update &args) {
    for (int id : args.cubes) {
        auto &cube = m_cubes.emplace(id, id).first->second;

        auto pos = cube_pile_offset(rng);
        cube.pos = sdl::point{
            parent->width() / 2 + pos.x + options::cube_pile_xoffset,
            parent->height() / 2 + pos.y
        };
    }
    pop_update();
}

void game_scene::HANDLE_UPDATE(move_cube, const move_cube_update &args) {
    auto [cube_it, inserted] = m_cubes.try_emplace(args.cube_id, args.cube_id);
    auto &cube = cube_it->second;
    if (cube.owner) {
        if (auto it = std::ranges::find(cube.owner->cubes, &cube); it != cube.owner->cubes.end()) {
            cube.owner->cubes.erase(it);
        }
    }
    sdl::point diff;
    if (args.card_id) {
        cube.owner = find_card(args.card_id);
        diff.x = options::cube_xdiff;
        diff.y = options::cube_ydiff + options::cube_yoff * cube.owner->cubes.size();
        cube.owner->cubes.push_back(&cube);
    } else {
        cube.owner = nullptr;
        diff = cube_pile_offset(rng);
        diff.x += parent->width() / 2 + options::cube_pile_xoffset;
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

void game_scene::HANDLE_UPDATE(show_card, const show_card_update &args) {
    card_view *card = find_card(args.info.id);

    if (card && !card->known) {
        *static_cast<card_data *>(card) = args.info;
        card->known = true;

        if (!card->image.empty()) {
            card->make_texture_front();
        }

        if (card->pile == &m_main_deck) {
            std::swap(*std::ranges::find(m_main_deck, card), m_main_deck.back());
        } else if (card->pile == &m_shop_deck) {
            std::swap(*std::ranges::find(m_shop_deck, card), m_shop_deck.back());
        } else if (card->pile == &m_specials) {
            m_ui.add_special(card);
        }
        if (bool(args.flags & show_card_flags::no_animation)) {
            card->flip_amt = 1.f;
            pop_update();
        } else {
            m_animations.emplace_back(10, std::in_place_type<card_flip_animation>, card, false);

            if (bool(args.flags & show_card_flags::short_pause)) {
                m_animations.emplace_back(20, std::in_place_type<pause_animation>, card);
            }
        }

    } else {
        pop_update();
    }
}

void game_scene::HANDLE_UPDATE(hide_card, const hide_card_update &args) {
    card_view *card = find_card(args.card_id);

    if (card && card->known && (m_player_own_id == 0 || args.ignore_player_id != m_player_own_id)) {
        if (card->pile == &m_specials) {
            m_ui.remove_special(card);
        }
        card->known = false;
        if (bool(args.flags & show_card_flags::no_animation)) {
            card->flip_amt = 0.f;
            pop_update();
        } else {
            if (bool(args.flags & show_card_flags::short_pause)) {
                m_animations.emplace_back(20, std::in_place_type<pause_animation>, card);
            }

            m_animations.emplace_back(10, std::in_place_type<card_flip_animation>, card, true);
        }
    } else {
        pop_update();
    }
}

void game_scene::HANDLE_UPDATE(tap_card, const tap_card_update &args) {
    card_view *card = find_card(args.card_id);
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

void game_scene::HANDLE_UPDATE(last_played_card, const card_id_args &args) {
    m_target.set_last_played_card(find_card(args.card_id));
    pop_update();
}

void game_scene::HANDLE_UPDATE(force_play_card, const card_id_args &args) {
    m_target.set_forced_card(find_card(args.card_id));
    pop_update();
}

void game_scene::move_player_views() {
    auto own_player = m_player_own_id ? m_players.find(m_player_own_id) : m_players.begin();
    if (own_player == m_players.end()) return;

    int xradius = (parent->width() / 2) - options::player_ellipse_x_distance;
    int yradius = (parent->height() / 2) - options::player_ellipse_y_distance;

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
        m_scenario_deck.set_pos(sdl::point{
            player_rect.x + player_rect.w + options::scenario_deck_xoff,
            player_rect.y + player_rect.h / 2});
    }
}

void game_scene::HANDLE_UPDATE(player_add, const player_user_update &args) {
    if (args.user_id == parent->get_user_own_id()) {
        m_player_own_id = args.player_id;
    }
    auto &p = m_players.try_emplace(args.player_id, args.player_id).first->second;
    p.m_role.texture_back = &card_textures::get().backface_role;

    p.user_id = args.user_id;
    if (user_info *info = parent->get_user_info(args.user_id)) {
        p.set_username(info->name);
        p.m_propic.set_texture(info->profile_image);
    } else {
        p.set_username(_("USERNAME_DISCONNECTED"));
        p.m_propic.set_texture(media_pak::get().icon_disconnected);
    }

    move_player_views();
    pop_update();
}

void game_scene::HANDLE_UPDATE(player_hp, const player_hp_update &args) {
    player_view *player = find_player(args.player_id);
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

void game_scene::HANDLE_UPDATE(player_gold, const player_gold_update &args) {
    find_player(args.player_id)->set_gold(args.gold);
    pop_update();
}

void game_scene::HANDLE_UPDATE(player_show_role, const player_show_role_update &args) {
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

void game_scene::HANDLE_UPDATE(player_status, const player_status_update &args) {
    if (player_view *p = find_player(args.player_id)) {
        p->m_player_flags = args.flags;
        p->m_range_mod = args.range_mod;
        p->m_weapon_range = args.weapon_range;
        p->m_distance_mod = args.distance_mod;
    }

    pop_update();
}

void game_scene::HANDLE_UPDATE(switch_turn, const switch_turn_update &args) {
    if (m_playing_id) {
        find_player(m_playing_id)->border_color = 0;
    }
    m_playing_id = args.player_id;
    if (m_playing_id) {
        find_player(m_playing_id)->border_color = options::turn_indicator_rgba;
    }

    m_target.clear_targets();

    pop_update();
}

void game_scene::HANDLE_UPDATE(request_status, const request_status_args &args) {
    m_request_origin_id = args.origin_id;
    m_request_target_id = args.target_id;
    m_target.set_response_highlights(args);

    m_ui.set_status(evaluate_format_string(args.status_text));

    pop_update();
}

void game_scene::HANDLE_UPDATE(status_clear) {
    m_request_origin_id = 0;
    m_request_target_id = 0;

    m_ui.clear_status();
    m_target.clear_status();

    pop_update();
}

void game_scene::HANDLE_UPDATE(confirm_play) {
    m_target.confirm_play();
    pop_update();
}