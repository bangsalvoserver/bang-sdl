#include "game.h"
#include "../manager.h"
#include "../media_pak.h"

#include <iostream>
#include <numbers>
#include <ranges>

using namespace banggame;
using namespace sdl::point_math;

game_scene::game_scene(client_manager *parent)
    : scene_base(parent)
    , m_card_textures(parent->get_base_path(), parent->get_renderer())
    , m_ui(this)
    , m_target(this)
{
    m_ui.enable_golobby(false);
}

card_view *game_scene::find_card(int id) const {
    if (auto it = m_cards.find(id); it != m_cards.end()) {
        return &*it;
    }
    throw std::runtime_error(fmt::format("client.find_card: ID {} not found", id));
}

player_view *game_scene::find_player(int id) const {
    if (auto it = m_players.find(id); it != m_players.end()) {
        return &*it;
    }
    throw std::runtime_error(fmt::format("client.find_player: ID {} not found", id));
}

void game_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();

    m_main_deck.set_pos(sdl::point{
        win_rect.w / 2 + options.deck_xoffset,
        win_rect.h / 2});

    m_discard_pile.set_pos(m_main_deck.get_pos() - sdl::point{options.discard_xoffset, 0});
    
    m_selection.set_pos(sdl::point{
        win_rect.w / 2,
        win_rect.h / 2 + options.selection_yoffset});

    m_shop_deck.set_pos(sdl::point{
        win_rect.w / 2 + options.shop_xoffset - options.shop_selection_width - options.card_width,
        win_rect.h / 2});

    m_shop_discard.set_pos(m_shop_deck.get_pos());

    m_shop_selection.set_pos(sdl::point{
        win_rect.w / 2 + options.shop_xoffset - options.shop_selection_width / 2,
        win_rect.h / 2});
    
    m_shop_choice.set_pos(m_shop_selection.get_pos() + sdl::point{0, options.shop_choice_offset});

    m_scenario_card.set_pos(sdl::point{
        win_rect.w / 2 + options.deck_xoffset + options.card_width + options.card_pocket_xoff,
        win_rect.h / 2});

    move_player_views();

    m_cubes.set_pos(sdl::point{
        win_rect.w / 2 + options.cube_pile_xoffset,
        win_rect.h / 2
    });

    m_ui.refresh_layout();

    m_button_row.set_pos(sdl::point{win_rect.w / 2, win_rect.h - 40});
}

void game_scene::tick(duration_type time_elapsed) {
    if (m_mouse_motion_timer >= durations.card_overlay) {
        if (!m_overlay) {
            find_overlay();
        }
    } else {
        if (!m_middle_click) {
            m_overlay = nullptr;
        }
        m_mouse_motion_timer += time_elapsed;
    }

    try {
        anim_duration_type tick_time{time_elapsed};
        while (true) {
            if (m_animations.empty()) {
                if (!m_pending_updates.empty()) {
                    enums::visit_indexed([this](auto && ... args) {
                        handle_game_update(FWD(args) ...);
                    }, json::deserialize<banggame::game_update>(m_pending_updates.front(), *this));
                    m_pending_updates.pop_front();
                } else {
                    break;
                }
            } else {
                auto &anim = m_animations.front();
                anim.tick(tick_time);
                if (anim.done()) {
                    tick_time = anim.extra_time();

                    anim.end();
                    m_animations.pop_front();
                } else {
                    break;
                }
            }
        }
    } catch (const std::exception &error) {
        parent->add_chat_message(message_type::error, fmt::format("Error: {}", error.what()));
        parent->disconnect();
    }
}

void game_scene::render(sdl::renderer &renderer) {
    m_main_deck.render_last(renderer, 2);
    m_shop_discard.render_first(renderer, 1);
    m_shop_deck.render_last(renderer, 2);
    m_shop_selection.render(renderer);
    m_scenario_card.render_last(renderer, 2);
    m_discard_pile.render_last(renderer, 2);
    m_cubes.render(renderer);

    for (player_view *p : m_alive_players) {
        p->render(renderer);
    }

    m_selection.render(renderer);
    m_shop_choice.render(renderer);

    for (player_view *p : m_dead_players) {
        p->render(renderer);
    }
    if (!m_dead_players.empty()) {
        sdl::texture_ref icon = media_pak::get().icon_dead_players;
        sdl::rect icon_rect = icon.get_rect();
        icon_rect.x = parent->get_rect().w - options.pile_dead_players_xoff - icon_rect.w / 2;
        icon_rect.y = options.icon_dead_players_yoff;
        icon.render_colored(renderer, icon_rect, colors.icon_dead_players);
    }

    if (!m_animations.empty()) {
        m_animations.front().render(renderer);
    }

    m_ui.render(renderer);
    m_button_row.render(renderer);

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
        m_mouse_motion_timer = duration_type{0};
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
    if (card_view *card = m_selection.find_card_at(m_mouse_pt)) {
        m_target.on_click_card(pocket_type::selection, nullptr, card);
        return;
    }
    if (card_view *card = (m_shop_choice.empty() ? m_shop_selection : m_shop_choice).find_card_at(m_mouse_pt)) {
        m_target.on_click_card(pocket_type::shop_selection, nullptr, card);
        return;
    }
    if (m_main_deck.find_card_at(m_mouse_pt)) {
        m_target.on_click_card(pocket_type::main_deck, nullptr, nullptr);
        return;
    }
    if (m_discard_pile.find_card_at(m_mouse_pt)) {
        m_target.on_click_card(pocket_type::discard_pile, nullptr, nullptr);
        return;
    }
    if (card_view *card = m_scenario_card.find_card_at(m_mouse_pt)) {
        m_target.on_click_card(pocket_type::scenario_card, nullptr, card);
        return;
    }
    for (player_view *p : m_alive_players) {
        if (sdl::point_in_rect(m_mouse_pt, p->m_bounding_rect)) {
            if (m_target.on_click_player(p)) {
                return;
            }
        }
        if (card_view *card = p->m_characters.find_card_at(m_mouse_pt)) {
            m_target.on_click_card(pocket_type::player_character, p, card);
            return;
        }
        if (card_view *card = p->table.find_card_at(m_mouse_pt)) {
            m_target.on_click_card(pocket_type::player_table, p, card);
            return;
        }
        if (card_view *card = p->hand.find_card_at(m_mouse_pt)) {
            m_target.on_click_card(pocket_type::player_hand, p, card);
            return;
        }
    }
}

void game_scene::find_overlay() {
    if (m_overlay = m_selection.find_card_at(m_mouse_pt)) {
        return;
    }
    if (m_overlay = m_shop_choice.find_card_at(m_mouse_pt)) {
        return;
    }
    if (m_overlay = m_shop_selection.find_card_at(m_mouse_pt)) {
        return;
    }
    if (m_overlay = m_discard_pile.find_card_at(m_mouse_pt)) {
        return;
    }
    if (m_overlay = m_scenario_card.find_card_at(m_mouse_pt)) {
        return;
    }
    for (player_view *p : m_alive_players) {
        if (m_overlay = p->m_characters.find_card_at(m_mouse_pt)) {
            return;
        }
        if (m_overlay = p->scenario_deck.find_card_at(m_mouse_pt)) {
            return;
        }
        if (sdl::point_in_rect(m_mouse_pt, p->m_role.get_rect())) {
            m_overlay = &p->m_role;
            return;
        }
        if (m_overlay = p->table.find_card_at(m_mouse_pt)) {
            return;
        }
        if (m_overlay = p->hand.find_card_at(m_mouse_pt)) {
            return;
        }
    }
    for (player_view *p : m_dead_players | std::views::reverse) {
        if (sdl::point_in_rect(m_mouse_pt, p->m_role.get_rect())) {
            m_overlay = &p->m_role;
            return;
        }
    }
}

void game_scene::handle_message(SRV_TAG(game_update), const Json::Value &update) {
    m_pending_updates.push_back(update);
}

void game_scene::handle_game_update(UPD_TAG(game_over), const game_over_update &args) {
    m_target.clear_status();

    m_ui.set_status(_("STATUS_GAME_OVER"));
    m_winner_role = args.winner_role;

    handle_message(SRV_TAG(lobby_owner){}, {parent->get_lobby_owner_id()});
}

void game_scene::handle_message(SRV_TAG(lobby_owner), const user_id_args &args) {
    if (m_winner_role != player_role::unknown) {
        m_ui.enable_golobby(parent->get_user_own_id() == args.user_id);
    }
}

void game_scene::handle_message(SRV_TAG(lobby_error), const std::string &message) {
    m_target.confirm_play();
}

std::string game_scene::evaluate_format_string(const game_string &str) {
    return intl::format(_(str.format_str),
        str.format_args | std::views::transform([&](const game_format_arg &arg) {
        return std::visit(overloaded{
            [](int value) { return std::to_string(value); },
            [](const std::string &value) { return _(value); },
            [&](card_format_id value) {
                if (value.sign) {
                    return intl::format("{} ({}{})", _(intl::category::cards, value.name), enums::get_data(value.sign.rank),
                        reinterpret_cast<const char *>(enums::get_data(value.sign.suit)));
                } else {
                    return _(intl::category::cards, value.name);
                }
            },
            [&](player_view *player) {
                return player ? player->m_username_text.get_value() : _("USERNAME_DISCONNECTED");
            }
        }, arg);
    }));
}

void game_scene::handle_game_update(UPD_TAG(game_error), const game_string &args) {
    m_target.confirm_play();
    parent->add_chat_message(message_type::error, evaluate_format_string(args));
}

void game_scene::handle_game_update(UPD_TAG(game_log), const game_string &args) {
    m_ui.add_game_log(evaluate_format_string(args));
}

void game_scene::handle_game_update(UPD_TAG(game_prompt), const game_string &args) {
    m_ui.show_message_box(evaluate_format_string(args),
        [&]{ m_target.send_prompt_response(true); },
        [&]{ m_target.send_prompt_response(false); }
    );
}

void game_scene::handle_game_update(UPD_TAG(deck_shuffled), const pocket_type &pocket) {
    switch (pocket) {
    case pocket_type::main_deck:
        for (card_view *card : m_discard_pile) {
            card->known = false;
            m_main_deck.add_card(card);
        }
        m_discard_pile.clear();
        add_animation<deck_shuffle_animation>(durations.shuffle_deck, &m_main_deck, m_discard_pile.get_pos());
        break;
    case pocket_type::shop_deck:
        for (card_view *card : m_shop_discard) {
            card->known = false;
            m_shop_deck.add_card(card);
        }
        m_shop_discard.clear();
        add_animation<deck_shuffle_animation>(durations.shuffle_deck, &m_shop_deck, m_shop_discard.get_pos());
        break;
    }
}

pocket_view &game_scene::get_pocket(pocket_type pocket, player_view *player) {
    switch(pocket) {
    case pocket_type::player_hand:       return player->hand;
    case pocket_type::player_table:      return player->table;
    case pocket_type::player_character:  return player->m_characters;
    case pocket_type::player_backup:     return player->m_backup_characters;
    case pocket_type::main_deck:         return m_main_deck;
    case pocket_type::discard_pile:      return m_discard_pile;
    case pocket_type::selection:         return m_selection;
    case pocket_type::shop_deck:         return m_shop_deck;
    case pocket_type::shop_selection:    return m_shop_selection;
    case pocket_type::shop_discard:      return m_shop_discard;
    case pocket_type::hidden_deck:       return m_hidden_deck;
    case pocket_type::scenario_deck:     return m_scenario_player->scenario_deck;
    case pocket_type::scenario_card:     return m_scenario_card;
    case pocket_type::button_row:        return m_button_row;
    default: throw std::runtime_error("Invalid pocket");
    }
}

void game_scene::handle_game_update(UPD_TAG(add_cards), const add_cards_update &args) {
    auto &pocket = get_pocket(args.pocket, args.player);

    for (auto [id, deck] : args.card_ids) {
        auto c = std::make_unique<card_view>();
        c->id = id;
        c->deck = deck;
        c->texture_back = card_textures::get().backfaces[enums::indexof(deck)];

        card_view *card = m_cards.insert(std::move(c)).get();
        pocket.add_card(card);
        card->set_pos(pocket.get_pos() + pocket.get_offset(card));
    }
}

void game_scene::handle_game_update(UPD_TAG(remove_cards), const remove_cards_update &args) {
    for (auto *c : args.cards) {
        if (c->pocket) {
            c->pocket->erase_card(c);
        }
        m_cards.erase(c->id);
    }
}

void game_scene::handle_game_update(UPD_TAG(move_card), const move_card_update &args) {
    pocket_view *old_pile = args.card->pocket;
    pocket_view *new_pile = &get_pocket(args.pocket, args.player);
    
    if (old_pile == new_pile) {
        return;
    }

    card_move_animation anim;

    old_pile->erase_card(args.card);
    if (old_pile->wide()) {
        for (card_view *anim_card : *old_pile) {
            anim.add_move_card(anim_card);
        }
    }

    new_pile->add_card(args.card);
    if (new_pile->wide()) {
        for (card_view *anim_card : *new_pile) {
            anim.add_move_card(anim_card);
        }
    } else {
        anim.add_move_card(args.card);
    }
    
    if (bool(args.flags & show_card_flags::instant)) {
        anim.end();
    } else {
        if (bool(args.flags & show_card_flags::pause_before_move)) {
            add_animation<pause_animation>(durations.short_pause, args.card);
        }
        
        add_animation<card_move_animation>(durations.move_card, std::move(anim));
    }
}

cube_pile_base &game_scene::get_cube_pile(card_view *card) {
    if (card) {
        return card->cubes;
    } else {
        return m_cubes;
    }
}

void game_scene::handle_game_update(UPD_TAG(add_cubes), const add_cubes_update &args) {
    auto &pile = get_cube_pile(args.target_card);
    for (int i=0; i<args.num_cubes; ++i) {
        auto &cube = pile.emplace_back(std::make_unique<cube_widget>());
        cube->pos = pile.get_pos() + pile.get_offset(cube.get());
    }
}

void game_scene::handle_game_update(UPD_TAG(move_cubes), const move_cubes_update &args) {
    auto &origin_pile = get_cube_pile(args.origin_card);
    auto &target_pile = get_cube_pile(args.target_card);

    cube_move_animation anim;
    for (int i=0; i<args.num_cubes; ++i) {
        auto &cube = target_pile.emplace_back(std::move(origin_pile.back()));
        origin_pile.pop_back();

        anim.add_cube(cube.get(), &target_pile);
    }
    add_animation<cube_move_animation>(args.num_cubes == 1 ? durations.move_cube : durations.move_cubes, std::move(anim));
}

void game_scene::handle_game_update(UPD_TAG(move_scenario_deck), player_view *player) {
    auto *old_scenario_player = std::exchange(m_scenario_player, player);
    if (old_scenario_player && !old_scenario_player->scenario_deck.empty()) {
        card_move_animation anim;
        for (card_view *c : old_scenario_player->scenario_deck) {
            m_scenario_player->scenario_deck.add_card(c);
            anim.add_move_card(c);
        }
        old_scenario_player->scenario_deck.clear();
        add_animation<card_move_animation>(durations.move_card, std::move(anim));
    }
}

void game_scene::handle_game_update(UPD_TAG(show_card), const show_card_update &args) {
    if (!args.card->known) {
        *static_cast<card_data *>(args.card) = args.info;
        args.card->known = true;

        if (!args.card->image.empty()) {
            args.card->make_texture_front(parent->get_renderer());
        }
        if (args.card->pocket) {
            args.card->pocket->update_card(args.card);
        }

        if (bool(args.flags & show_card_flags::instant)) {
            args.card->flip_amt = 1.f;
        } else {
            add_animation<card_flip_animation>(durations.flip_card, args.card, false);

            if (bool(args.flags & show_card_flags::short_pause)) {
                add_animation<pause_animation>(durations.short_pause, args.card);
            }
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(hide_card), const hide_card_update &args) {
    if (args.card->known) {
        args.card->known = false;
        if (bool(args.flags & show_card_flags::instant)) {
            args.card->flip_amt = 0.f;
        } else {
            if (bool(args.flags & show_card_flags::short_pause)) {
                add_animation<pause_animation>(durations.short_pause, args.card);
            }

            add_animation<card_flip_animation>(durations.flip_card, args.card, true);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(tap_card), const tap_card_update &args) {
    if (args.card->inactive != args.inactive) {
        args.card->inactive = args.inactive;
        if (args.instant) {
            args.card->rotation = args.card->inactive ? 90.f : 0.f;
        } else {
            add_animation<card_tap_animation>(durations.tap_card, args.card, args.inactive);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(flash_card), card_view *card) {
    add_animation<card_flash_animation>(durations.flash_card, card, colors.flash_card);
}

void game_scene::handle_game_update(UPD_TAG(last_played_card), card_view *card) {
    m_target.set_last_played_card(card);
}

void game_scene::move_player_views(bool instant) {
    if (m_alive_players.size() == 0) return;

    player_move_animation anim;

    const int xradius = (parent->width() / 2) - options.player_ellipse_x_distance;
    const int yradius = (parent->height() / 2) - options.player_ellipse_y_distance;
    
    double angle = 0.f;

    if (m_alive_players.size() == 1 && m_alive_players.front() != m_player_self) {
        angle = std::numbers::pi;
    }

    for (player_view *p : m_alive_players) {
        anim.add_move_player(p, sdl::point{
            int(parent->width() / 2 - std::sin(angle) * xradius),
            int(parent->height() / 2 + std::cos(angle) * yradius)
        });
        
        angle += std::numbers::pi * 2.f / m_alive_players.size();
    }

    sdl::point dead_roles_pos{
        parent->get_rect().w - options.pile_dead_players_xoff,
        options.pile_dead_players_yoff
    };
    for (player_view *p : m_dead_players) {
        anim.add_move_player(p, dead_roles_pos);
        dead_roles_pos.y += options.pile_dead_players_ydiff;
    }

    if (instant) {
        anim.end();
    } else {
        add_animation<player_move_animation>(durations.move_player, std::move(anim));
    }
}

void game_scene::handle_game_update(UPD_TAG(player_add), const player_add_update &args) {
    for (int player_id = 1; player_id <= args.num_players; ++player_id) {
        auto &p = m_players.emplace(this, player_id);
        
        p.m_role.texture_back = card_textures::get().backfaces[enums::indexof(card_deck_type::role)];

        m_alive_players.push_back(&p);
    }
}

void game_scene::handle_game_update(UPD_TAG(player_user), const player_user_update &args) {
    args.player->user_id = args.user_id;
    if (args.user_id == parent->get_user_own_id()) {
        m_player_self = args.player;
        auto alive_it = std::ranges::find(m_alive_players, args.player);
        if (alive_it != m_alive_players.end()) {
            std::ranges::rotate(m_alive_players, alive_it);
        }
    }

    if (user_info *info = parent->get_user_info(args.user_id)) {
        args.player->m_username_text.set_value(info->name);
        args.player->m_propic.set_texture(info->profile_image);
    } else {
        args.player->m_username_text.set_value(_("USERNAME_DISCONNECTED"));
        args.player->m_propic.set_texture(media_pak::get().icon_disconnected);
    }

    move_player_views();
}

void game_scene::handle_game_update(UPD_TAG(player_remove), const player_remove_update &args) {
    auto it = std::ranges::find(m_alive_players, args.player->id, &player_view::id);
    if (it != m_alive_players.end()) {
        m_alive_players.erase(it);

        args.player->set_to_dead();

        args.player->set_position(args.player->m_role.get_pos() - sdl::point{(options.card_margin + widgets::profile_pic::size) / 2, 0});

        m_dead_players.push_back(args.player);

        move_player_views(args.instant);
    }
}

void game_scene::handle_game_update(UPD_TAG(player_hp), const player_hp_update &args) {
    int prev_hp = args.player->hp;
    args.player->hp = args.hp;
    if (args.instant) {
        args.player->set_hp_marker_position(static_cast<float>(args.hp));
    } else if (prev_hp != args.hp) {
        add_animation<player_hp_animation>(durations.move_hp, args.player, prev_hp);
    }
}

void game_scene::handle_game_update(UPD_TAG(player_gold), const player_gold_update &args) {
    args.player->set_gold(args.gold);
}

void game_scene::handle_game_update(UPD_TAG(player_show_role), const player_show_role_update &args) {
    if (args.player->m_role.role != args.role) {
        args.player->m_role.role = args.role;
        args.player->m_role.make_texture_front(parent->get_renderer());
        if (args.instant) {
            if (args.role == player_role::sheriff) {
                args.player->set_hp_marker_position(static_cast<float>(++args.player->hp));
            }
            args.player->m_role.flip_amt = 1.f;
        } else {
            add_animation<card_flip_animation>(durations.flip_role, &args.player->m_role, false);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(player_status), const player_status_update &args) {
    args.player->m_player_flags = args.flags;
    args.player->m_range_mod = args.range_mod;
    args.player->m_weapon_range = args.weapon_range;
    args.player->m_distance_mod = args.distance_mod;
}

void game_scene::handle_game_update(UPD_TAG(switch_turn), player_view *player) {
    m_target.clear_status();

    if (player != m_playing) {
        m_playing = player;
        if (m_playing) {
            m_turn_border = {m_playing->border_color, colors.turn_indicator};
        } else {
            m_turn_border = {};
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(request_status), const request_status_args &args) {
    m_request_origin = args.origin;
    m_request_target = args.target;
    m_target.set_response_highlights(args);

    m_ui.set_status(evaluate_format_string(args.status_text));
}

void game_scene::handle_game_update(UPD_TAG(game_flags), const game_flags &args) {
    m_game_flags = args;
}

void game_scene::handle_game_update(UPD_TAG(play_sound), const std::string &sound_id) {
    parent->play_sound(sound_id);
}

void game_scene::handle_game_update(UPD_TAG(status_clear)) {
    m_request_origin = nullptr;
    m_request_target = nullptr;

    m_ui.clear_status();
    m_target.clear_status();
}

void game_scene::handle_game_update(UPD_TAG(confirm_play)) {
    m_target.confirm_play();
}