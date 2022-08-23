#include "game.h"
#include "../manager.h"
#include "../media_pak.h"

#include <iostream>
#include <numbers>
#include <ranges>

using namespace banggame;
using namespace enums::flag_operators;
using namespace sdl::point_math;

template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

game_scene::game_scene(client_manager *parent)
    : scene_base(parent)
    , m_card_textures(parent->get_base_path(), parent->get_renderer())
    , m_ui(this)
    , m_target(this)
{
    m_ui.enable_golobby(false);
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
        win_rect.w / 2 + options.deck_xoffset + options.card_width + options.card_xoffset,
        win_rect.h / 2});

    move_player_views();

    m_cubes.set_pos(sdl::point{
        win_rect.w / 2 + options.cube_pile_xoffset,
        win_rect.h / 2
    });

    m_ui.refresh_layout();
}

void game_scene::tick(duration_type time_elapsed) {
    if (m_mouse_motion_timer >= std::chrono::milliseconds{options.card_overlay_msecs}) {
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
                    }, m_pending_updates.front());
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
        icon.render_colored(renderer, icon_rect, options.icon_dead_players);
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
    auto mouse_in_card = [&](card_view *card) {
        return sdl::point_in_rect(m_mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const pocket_view &pocket) {
        auto it = std::ranges::find_if(pocket | std::views::reverse, mouse_in_card);
        return (it == pocket.rend()) ? nullptr : *it;
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
    for (player_view *p : m_alive_players) {
        if (sdl::point_in_rect(m_mouse_pt, p->m_bounding_rect)) {
            if (m_target.on_click_player(p)) {
                return;
            }
        }
        for (card_view *c : p->m_characters | std::views::reverse) {
            if (sdl::point_in_rect(m_mouse_pt, c->get_rect())) {
                m_target.on_click_character(p, c);
                return;
            }
        }
        if (card_view *card = find_clicked(p->table)) {
            m_target.on_click_table_card(p, card);
            return;
        }
        if (card_view *card = find_clicked(p->hand)) {
            m_target.on_click_hand_card(p, card);
            return;
        }
    }
}

void game_scene::find_overlay() {
    auto mouse_in_card = [&](const card_view *card) {
        return sdl::point_in_rect(m_mouse_pt, card->get_rect());
    };
    auto find_clicked = [&](const pocket_view &pocket) {
        auto it = std::ranges::find_if(pocket | std::views::reverse,
            [&](const card_view *card) {
                return mouse_in_card(card) && card->known;
            }
        );
        return (it == pocket.rend()) ? nullptr : *it;
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
    if (!m_scenario_card.empty() && mouse_in_card(m_scenario_card.back())) {
        m_overlay = m_scenario_card.back();
        return;
    }
    for (player_view *p : m_alive_players) {
        for (card_view *c : p->m_characters | std::views::reverse) {
            if (sdl::point_in_rect(m_mouse_pt, c->get_rect())) {
                m_overlay = c;
                return;
            }
        }
        if (!p->scenario_deck.empty() && mouse_in_card(p->scenario_deck.back())) {
            m_overlay = p->scenario_deck.back();
            return;
        }
        if (mouse_in_card(p->m_role)) {
            m_overlay = p->m_role;
            return;
        }
        if (m_overlay = find_clicked(p->table)) {
            return;
        }
        if (m_overlay = find_clicked(p->hand)) {
            return;
        }
    }
}

void game_scene::handle_game_update(const game_update &update) {
#ifdef DEBUG_PRINT_GAME_UPDATES
    std::cout << "/*** GAME UPDATE ***/ " << json::serialize(update) << '\n';
#endif
    m_pending_updates.push_back(update);
}

void game_scene::handle_game_update(UPD_TAG(game_over), const game_over_update &args) {
    m_target.clear_status();

    m_ui.set_status(_("STATUS_GAME_OVER"));
    m_winner_role = args.winner_role;
    
    if (parent->get_user_own_id() == parent->get_lobby_owner_id()) {
        m_ui.enable_golobby(true);
    }
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
            [&](player_format_id value) {
                player_view *p = find_player(value.player_id);
                return p ? p->m_username_text.get_value() : _("USERNAME_DISCONNECTED");
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
    case pocket_type::main_deck: {
        card_view *top_discard = m_discard_pile.back();
        if (m_game_options.keep_last_card_shuffling) {
            m_discard_pile.erase_card(top_discard);
        }
        for (card_view *card : m_discard_pile) {
            card->known = false;
            m_main_deck.add_card(card);
        }
        m_discard_pile.clear();
        if (m_game_options.keep_last_card_shuffling) {
            m_discard_pile.add_card(top_discard);
        }
        add_animation<deck_shuffle_animation>(options.shuffle_deck_msecs, &m_main_deck, m_discard_pile.get_pos());
        break;
    }
    case pocket_type::shop_deck:
        for (card_view *card : m_shop_discard) {
            card->known = false;
            m_shop_deck.add_card(card);
        }
        m_shop_discard.clear();
        add_animation<deck_shuffle_animation>(options.shuffle_deck_msecs, &m_shop_deck, m_shop_discard.get_pos());
        break;
    }
}

pocket_view &game_scene::get_pocket(pocket_type pocket, int player_id) {
    switch(pocket) {
    case pocket_type::player_hand:       return find_player(player_id)->hand;
    case pocket_type::player_table:      return find_player(player_id)->table;
    case pocket_type::player_character:  return find_player(player_id)->m_characters;
    case pocket_type::player_backup:     return find_player(player_id)->m_backup_characters;
    case pocket_type::main_deck:         return m_main_deck;
    case pocket_type::discard_pile:      return m_discard_pile;
    case pocket_type::selection:         return m_selection;
    case pocket_type::shop_deck:         return m_shop_deck;
    case pocket_type::shop_selection:    return m_shop_selection;
    case pocket_type::shop_discard:      return m_shop_discard;
    case pocket_type::hidden_deck:       return m_hidden_deck;
    case pocket_type::scenario_deck:     return m_scenario_player->scenario_deck;
    case pocket_type::scenario_card:     return m_scenario_card;
    case pocket_type::specials:          return m_specials;
    default: throw std::runtime_error("Invalid pocket");
    }
}

void game_scene::handle_game_update(UPD_TAG(add_cards), const add_cards_update &args) {
    auto &pocket = get_pocket(args.pocket, args.player_id);

    for (auto [id, deck] : args.card_ids) {
        auto c = std::make_unique<card_view>();
        c->id = id;
        c->deck = deck;
        c->texture_back = card_textures::get().backfaces[enums::indexof(deck)];

        card_view *card_ptr = m_cards.insert(std::move(c)).get();
        pocket.add_card(card_ptr);
        card_ptr->set_pos(pocket.get_pos() + pocket.get_offset(card_ptr));
    }
}

void game_scene::handle_game_update(UPD_TAG(remove_cards), const remove_cards_update &args) {
    for (auto [id, deck] : args.card_ids) {
        auto *c = find_card(id);
        if (c && c->pocket) {
            c->pocket->erase_card(c);
        }
        m_cards.erase(id);
    }
}

void game_scene::handle_game_update(UPD_TAG(move_card), const move_card_update &args) {
    card_view *card = find_card(args.card_id);
    if (!card) {
        return;
    }

    pocket_view *old_pile = card->pocket;
    pocket_view *new_pile = &get_pocket(args.pocket, args.player_id);
    
    if (old_pile == new_pile) {
        return;
    }

    card_move_animation anim;

    old_pile->erase_card(card);
    if (old_pile->wide()) {
        for (card_view *anim_card : *old_pile) {
            anim.add_move_card(anim_card);
        }
    }

    new_pile->add_card(card);
    if (new_pile->wide()) {
        for (card_view *anim_card : *new_pile) {
            anim.add_move_card(anim_card);
        }
    } else {
        anim.add_move_card(card);
    }
    
    if (bool(args.flags & show_card_flags::instant)) {
        anim.end();
    } else {
        if (bool(args.flags & show_card_flags::pause_before_move)) {
            add_animation<pause_animation>(options.short_pause_msecs, card);
        }
        
        add_animation<card_move_animation>(options.move_card_msecs, std::move(anim));
    }
}

cube_pile_base &game_scene::find_cube_pile(int card_id) {
    if (card_id) {
        return find_card(card_id)->cubes;
    } else {
        return m_cubes;
    }
}

void game_scene::handle_game_update(UPD_TAG(add_cubes), const add_cubes_update &args) {
    auto &pile = find_cube_pile(args.target_card_id);
    for (int i=0; i<args.num_cubes; ++i) {
        auto &cube = pile.emplace_back(std::make_unique<cube_widget>());
        cube->pos = pile.get_pos() + pile.get_offset(cube.get());
    }
}

void game_scene::handle_game_update(UPD_TAG(move_cubes), const move_cubes_update &args) {
    auto &origin_pile = find_cube_pile(args.origin_card_id);
    auto &target_pile = find_cube_pile(args.target_card_id);

    cube_move_animation anim;
    for (int i=0; i<args.num_cubes; ++i) {
        auto &cube = target_pile.emplace_back(std::move(origin_pile.back()));
        origin_pile.pop_back();

        anim.add_cube(cube.get(), &target_pile);
    }
    add_animation<cube_move_animation>(args.num_cubes == 1 ? options.move_cube_msecs : options.move_cubes_msecs, std::move(anim));
}

void game_scene::handle_game_update(UPD_TAG(move_scenario_deck), const move_scenario_deck_args &args) {
    auto *old_scenario_player = std::exchange(m_scenario_player, find_player(args.player_id));
    if (old_scenario_player && !old_scenario_player->scenario_deck.empty()) {
        card_move_animation anim;
        for (card_view *c : old_scenario_player->scenario_deck) {
            m_scenario_player->scenario_deck.add_card(c);
            anim.add_move_card(c);
        }
        old_scenario_player->scenario_deck.clear();
        add_animation<card_move_animation>(options.move_card_msecs, std::move(anim));
    }
}

void game_scene::handle_game_update(UPD_TAG(show_card), const show_card_update &args) {
    card_view *card = find_card(args.info.id);

    if (card && !card->known) {
        *static_cast<card_data *>(card) = args.info;
        card->known = true;

        if (!card->image.empty()) {
            card->make_texture_front(parent->get_renderer());
        }

        if (card->pocket == &m_main_deck) {
            std::swap(*std::ranges::find(m_main_deck, card), m_main_deck.back());
        } else if (card->pocket == &m_shop_deck) {
            std::swap(*std::ranges::find(m_shop_deck, card), m_shop_deck.back());
        } else if (card->pocket == &m_specials) {
            m_ui.add_special(card);
        }
        if (bool(args.flags & show_card_flags::instant)) {
            card->flip_amt = 1.f;
        } else {
            add_animation<card_flip_animation>(options.flip_card_msecs, card, false);

            if (bool(args.flags & show_card_flags::short_pause)) {
                add_animation<pause_animation>(options.short_pause_msecs, card);
            }
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(hide_card), const hide_card_update &args) {
    card_view *card = find_card(args.card_id);

    if (card && card->known) {
        if (card->pocket == &m_specials) {
            m_ui.remove_special(card);
        }
        card->known = false;
        if (bool(args.flags & show_card_flags::instant)) {
            card->flip_amt = 0.f;
        } else {
            if (bool(args.flags & show_card_flags::short_pause)) {
                add_animation<pause_animation>(options.short_pause_msecs, card);
            }

            add_animation<card_flip_animation>(options.flip_card_msecs, card, true);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(tap_card), const tap_card_update &args) {
    card_view *card = find_card(args.card_id);
    if (card->inactive != args.inactive) {
        card->inactive = args.inactive;
        if (args.instant) {
            card->rotation = card->inactive ? 90.f : 0.f;
        } else {
            add_animation<card_tap_animation>(options.tap_card_msecs, card, args.inactive);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(flash_card), const flash_card_update &args) {
    add_animation<card_flash_animation>(options.flash_card_msecs, find_card(args.card_id), options.flash_card_color);
}

void game_scene::handle_game_update(UPD_TAG(last_played_card), const card_id_args &args) {
    m_target.set_last_played_card(find_card(args.card_id));
}

void game_scene::move_player_views(bool do_animation) {
    if (m_alive_players.size() == 0) return;

    player_move_animation anim;

    const int xradius = (parent->width() / 2) - options.player_ellipse_x_distance;
    const int yradius = (parent->height() / 2) - options.player_ellipse_y_distance;

    double angle = 0.f;
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

    if (do_animation) {
        add_animation<player_move_animation>(options.move_player_msecs, std::move(anim));
    } else {
        anim.end();
    }
}

void game_scene::handle_game_update(UPD_TAG(player_add), const player_user_update &args) {
    auto [p, inserted] = m_players.try_insert(std::make_unique<alive_player_view>(this, args.player_id));
    if (inserted) {
        auto card = std::make_unique<role_card>();
        card->id = args.player_id;
        card->texture_back = card_textures::get().backfaces[enums::indexof(card_deck_type::role)];
        p.m_role = m_role_cards.insert(std::move(card)).get();

        auto it = m_alive_players.insert(m_alive_players.empty()
            ? m_alive_players.end()
            : std::next(std::ranges::max_element(m_alive_players, {}, &player_view::id)), &p);
        
        if (args.user_id == parent->get_user_own_id()) {
            m_player_self = &p;
            std::ranges::rotate(m_alive_players, it);
        }
    }

    p.user_id = args.user_id;
    if (user_info *info = parent->get_user_info(args.user_id)) {
        p.set_username(info->name);
        p.m_propic.set_texture(info->profile_image);
    } else {
        p.set_username(_("USERNAME_DISCONNECTED"));
        p.m_propic.set_texture(media_pak::get().icon_disconnected);
    }

    move_player_views();
}

void game_scene::handle_game_update(UPD_TAG(player_remove), const player_remove_update &args) {
    if (m_player_self && m_player_self->id == args.player_id) {
        m_player_self = nullptr;
    }

    auto it = std::ranges::find(m_alive_players, args.player_id, &player_view::id);
    if (it != m_alive_players.end()) {
        player_view *alive_player = *it;
        m_alive_players.erase(it);

        auto dead_player = std::make_unique<dead_player_view>(this, args.player_id);
        dead_player->m_role = alive_player->m_role;
        dead_player->set_username(alive_player->m_username_text.get_value());
        dead_player->m_propic.set_texture(alive_player->m_propic.get_texture());
        dead_player->m_player_flags = player_flags::dead;
        dead_player->set_position(alive_player->m_role->get_pos() - sdl::point{
            (options.card_margin + widgets::profile_pic::size) / 2,
            0
        });

        m_dead_players.push_back(m_players.insert(std::move(dead_player)).get());

        move_player_views(true);
    }
}

void game_scene::handle_game_update(UPD_TAG(player_hp), const player_hp_update &args) {
    player_view *player = find_player(args.player_id);
    int prev_hp = player->hp;
    player->hp = args.hp;
    if (args.instant) {
        player->set_hp_marker_position(static_cast<float>(args.hp));
    } else if (prev_hp != args.hp) {
        add_animation<player_hp_animation>(options.move_hp_msecs, player, prev_hp);
    }
}

void game_scene::handle_game_update(UPD_TAG(player_gold), const player_gold_update &args) {
    find_player(args.player_id)->set_gold(args.gold);
}

void game_scene::handle_game_update(UPD_TAG(player_show_role), const player_show_role_update &args) {
    if (auto *p = find_player(args.player_id)) {
        if (p->m_role->role != args.role) {
            p->m_role->role = args.role;
            p->m_role->make_texture_front(parent->get_renderer());
            if (args.instant) {
                if (args.role == player_role::sheriff) {
                    p->set_hp_marker_position(static_cast<float>(++p->hp));
                }
                p->m_role->flip_amt = 1.f;
            } else {
                add_animation<card_flip_animation>(options.flip_role_msecs, p->m_role, false);
            }
        }
        move_player_views();
    } else {
        auto card = std::make_unique<role_card>();
        card->id = args.player_id;
        card->flip_amt = 1.f;
        card->role = args.role;
        card->make_texture_front(parent->get_renderer());
        m_role_cards.insert(std::move(card));
    }
}

void game_scene::handle_game_update(UPD_TAG(player_status), const player_status_update &args) {
    if (player_view *p = find_player(args.player_id)) {
        p->m_player_flags = args.flags;
        p->m_range_mod = args.range_mod;
        p->m_weapon_range = args.weapon_range;
        p->m_distance_mod = args.distance_mod;
    }
}

void game_scene::handle_game_update(UPD_TAG(switch_turn), const switch_turn_update &args) {
    m_target.clear_status();

    player_view *new_player = find_player(args.player_id);

    if (new_player != m_playing) {
        m_playing = new_player;
        if (m_playing) {
            m_turn_border = {m_playing->border_color, options.turn_indicator};
        } else {
            m_turn_border = {};
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(request_status), const request_status_args &args) {
    m_request_origin = find_player(args.origin_id);
    m_request_target = find_player(args.target_id);
    m_target.set_response_highlights(args);

    m_ui.set_status(evaluate_format_string(args.status_text));
}

void game_scene::handle_game_update(UPD_TAG(game_flags), const game_flags &args) {
    m_game_flags = args;
}

void game_scene::handle_game_update(UPD_TAG(game_options), const game_options &args) {
    m_game_options = args;
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