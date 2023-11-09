#include "game.h"
#include "../manager.h"
#include "../media_pak.h"

#include "cards/effect_enums.h"

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
    if (parent->get_config().sound_volume > 0) {
        m_sounds.emplace(parent->get_base_path());
    }
    m_ui.enable_golobby(false);
}

void game_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();
    const sdl::point win_center { win_rect.w / 2, win_rect.h / 2 };

    m_button_row.set_pos(sdl::point{win_rect.w / 2, win_rect.h - options.button_row_yoffset});
    m_ui.refresh_layout();

    m_main_deck.set_pos(win_center + options.deck_offset);
    m_discard_pile.set_pos(m_main_deck.get_pos() + options.discard_offset);

    m_scenario_deck.set_pos(m_main_deck.get_pos() + options.scenario_deck_offset);
    m_scenario_card.set_pos(m_scenario_deck.get_pos() + options.scenario_card_offset);

    m_wws_scenario_deck.set_pos(m_scenario_deck.get_pos() + options.card_diag_offset);
    m_wws_scenario_card.set_pos(m_wws_scenario_deck.get_pos() + options.scenario_card_offset);

    m_cubes.set_pos(win_center + options.cube_pile_offset);

    m_shop_deck.set_pos(win_center + options.shop_deck_offset);
    m_shop_discard.set_pos(m_shop_deck.get_pos());
    m_shop_selection.set_pos(m_shop_deck.get_pos() + options.shop_selection_offset);

    m_train_deck.set_pos(win_center + options.train_deck_offset);
    m_stations.set_pos(m_train_deck.get_pos() + options.stations_offset);
    m_train.set_pos(m_stations.get_pos() + options.train_offset + options.train_card_offset * m_train_position);
    
    m_selection.set_pos(win_center + options.selection_offset);

    move_player_views();
    
    if (card_view *anchor = m_card_choice.get_anchor()) {
        m_card_choice.set_pos(anchor->get_pos());
    }
}

void game_scene::tick(duration_type time_elapsed) {
    if (m_mouse_motion_timer >= options.card_overlay_duration) {
        if (!m_overlay) {
            m_overlay = std::get<card_view *>(find_card_at(m_mouse_pt));
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
                    }, json::deserialize<banggame::game_update>(m_pending_updates.front(), context()));
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
    m_scenario_deck.render_last(renderer, 1);
    m_scenario_card.render_last(renderer, 2);
    m_wws_scenario_deck.render_last(renderer, 1);
    m_wws_scenario_card.render_last(renderer, 2);
    m_discard_pile.render_last(renderer, 2);
    m_stations.render(renderer);
    m_train_deck.render_last(renderer, 2);
    m_train.render(renderer);
    m_cubes.render(renderer);

    for (player_view *p : m_alive_players) {
        p->render(renderer);
    }

    m_selection.render(renderer);
    m_card_choice.render(renderer);

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

    if (m_overlay && m_overlay->known) {
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
                if (std::ranges::none_of(m_alive_players, [&](player_view *p) {
                    return sdl::point_in_rect(m_mouse_pt, p->m_bounding_rect)
                        && m_target.on_click_player(p);
                })) {
                    if (auto [pocket, player, card] = find_card_at(m_mouse_pt); pocket != pocket_type::none) {
                        m_target.on_click_card(pocket, player, card);
                    }
                }
            }
            break;
        case SDL_BUTTON_RIGHT:
            if (!m_target.finished() && !m_ui.is_message_box_open()) {
                m_target.clear_targets();
                m_target.handle_auto_select();
            }
            break;
        case SDL_BUTTON_MIDDLE:
            m_middle_click = true;
            m_overlay = std::get<card_view *>(find_card_at(m_mouse_pt));
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

std::tuple<pocket_type, player_view *, card_view *> game_scene::find_card_at(sdl::point pt) const {
    for (player_view *p : m_dead_players | std::views::reverse) {
        if (sdl::point_in_rect(pt, p->m_role.get_rect())) {
            return {pocket_type::none, p, &p->m_role};
        }
    }
    if (card_view *card = m_card_choice.find_card_at(pt)) {
        return {pocket_type::hidden_deck, nullptr, card};
    }
    if (card_view *card = m_selection.find_card_at(pt)) {
        return {pocket_type::selection, nullptr, card};
    }
    for (player_view *p : m_alive_players | std::views::reverse) {
        if (card_view *card = p->hand.find_card_at(pt)) {
            return {pocket_type::player_hand, p, card};
        }
        if (card_view *card = p->table.find_card_at(pt)) {
            return {pocket_type::player_table, p, card};
        }
        if (card_view *card = p->m_characters.find_card_at(pt)) {
            return {pocket_type::player_character, p, card};
        }
        if (sdl::point_in_rect(pt, p->m_role.get_rect())) {
            return {pocket_type::none, p, &p->m_role};
        }
    }
    if (card_view *card = m_train.find_card_at(pt)) {
        return {pocket_type::train, nullptr, card};
    }
    if (card_view *card = m_stations.find_card_at(pt)) {
        return {pocket_type::stations, nullptr, card};
    }
    if (card_view *card = m_discard_pile.find_card_at(pt)) {
        return {pocket_type::discard_pile, nullptr, card};
    }
    if (card_view *card = m_wws_scenario_card.find_card_at(pt)) {
        return {pocket_type::wws_scenario_card, nullptr, card};
    }
    if (card_view *card = m_wws_scenario_deck.find_card_at(pt)) {
        return {pocket_type::wws_scenario_deck, nullptr, card};
    }
    if (card_view *card = m_scenario_card.find_card_at(pt)) {
        return {pocket_type::scenario_card, nullptr, card};
    }
    if (card_view *card = m_scenario_deck.find_card_at(pt)) {
        return {pocket_type::scenario_deck, nullptr, card};
    }
    if (card_view *card = m_shop_selection.find_card_at(pt)) {
        return {pocket_type::shop_selection, nullptr, card};
    }
    if (card_view *card = m_main_deck.find_card_at(pt)) {
        return {pocket_type::main_deck, nullptr, nullptr};
    }
    return {pocket_type::none, nullptr, nullptr};
}

void game_scene::play_sound(std::string_view sound_id) {
    if (m_sounds) {
        m_sounds->play_sound(sound_id, parent->get_config().sound_volume);
    }
}

void game_scene::handle_message(SRV_TAG(game_update), const json::json &update) {
    m_pending_updates.push_back(update);
}

void game_scene::handle_message(SRV_TAG(lobby_owner), const user_id_args &args) {
    if (has_game_flags(game_flags::game_over)) {
        m_ui.enable_golobby(parent->get_user_own_id() == args.user_id);
    }
}

void game_scene::handle_message(SRV_TAG(lobby_error), const std::string &message) {
    m_target.clear_targets();
}

template<> class fmt::formatter<game_format_arg> {
private:
    std::string_view format_singular = "{}";
    std::string_view format_plural = "{}";

public:
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        if (ctx.begin() == ctx.end()) {
            return ctx.end();
        }

        auto end = ctx.begin();
        int curly_count = 1;
        while (curly_count != 0) {
            ++end;
            if (*end == '{') ++curly_count;
            else if (*end == '}') --curly_count;
        }
        std::string_view format_template{ctx.begin(), end};
        size_t semicolon_pos = format_template.find(';');
        if (semicolon_pos == std::string_view::npos) {
            throw fmt::format_error(fmt::format("Invalid format template {}", format_template));
        }
        format_singular = format_template.substr(0, semicolon_pos);
        format_plural = format_template.substr(semicolon_pos + 1);
        return end;
    }

    template<typename FormatContext>
    auto format(const game_format_arg &arg, FormatContext &ctx) {
        return ::enums::visit(overloaded{
            [&](int value) {
                std::string_view format_str = value == 1 ? format_singular : format_plural;
                return fmt::vformat_to(ctx.out(), format_str, fmt::make_format_args(value));
            },
            [&](const card_format &value) {
                if (value.sign) {
                    return fmt::format_to(ctx.out(), "{} ({}{})",
                        _(intl::category::cards, value.name),
                        ::enums::get_data(value.sign.rank),
                        reinterpret_cast<const char *>(::enums::get_data(value.sign.suit)));
                } else if (!value.name.empty()) {
                    return fmt::format_to(ctx.out(), "{}", _(intl::category::cards, value.name));
                } else {
                    return fmt::format_to(ctx.out(), "{}", _("UNKNOWN_CARD"));
                }
            },
            [&](player_view *player) {
                return fmt::format_to(ctx.out(), "{}", player ? player->m_username_text.get_value() : _("UNKNOWN_PLAYER"));
            }
        }, arg);
    }
};

std::string format_game_string(const banggame::game_string &str) {
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &arg: str.format_args) {
        store.push_back(arg);
    }

    std::string format_str = _(str.format_str);
    try {
        return fmt::vformat(format_str, store);
    } catch (const fmt::format_error &) {
        return format_str;
    }
}

void game_scene::handle_game_update(UPD_TAG(game_error), const game_string &args) {
    m_target.clear_targets();
    m_target.handle_auto_select();
    parent->add_chat_message(message_type::error, format_game_string(args));
    play_sound("invalid");
}

void game_scene::handle_game_update(UPD_TAG(game_log), const game_string &args) {
    m_ui.add_game_log(format_game_string(args));
}

void game_scene::handle_game_update(UPD_TAG(game_prompt), const game_string &args) {
    m_ui.show_message_box(format_game_string(args), {
        {_("BUTTON_YES"), [&]{ m_target.send_prompt_response(true); }},
        {_("BUTTON_NO"),  [&]{ m_target.send_prompt_response(false); }}
    });
}

void game_scene::handle_game_update(UPD_TAG(deck_shuffled), const deck_shuffled_update &args) {
    auto &from_pocket = get_pocket(args.pocket == pocket_type::main_deck ? pocket_type::discard_pile : pocket_type::shop_discard);
    auto &to_pocket = get_pocket(args.pocket);
    for (card_view *card : from_pocket) {
        card->known = false;
        to_pocket.add_card(card);
    }
    from_pocket.clear();
    add_animation<deck_shuffle_animation>(args.duration, &to_pocket, from_pocket.get_pos());
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
    case pocket_type::scenario_deck:     return m_scenario_deck;
    case pocket_type::scenario_card:     return m_scenario_card;
    case pocket_type::wws_scenario_deck: return m_wws_scenario_deck;
    case pocket_type::wws_scenario_card: return m_wws_scenario_card;
    case pocket_type::button_row:        return m_button_row;
    case pocket_type::stations:          return m_stations;
    case pocket_type::train:             return m_train;
    case pocket_type::train_deck:        return m_train_deck;
    default: throw std::runtime_error("Invalid pocket");
    }
}

void game_scene::handle_game_update(UPD_TAG(add_cards), const add_cards_update &args) {
    auto &pocket = get_pocket(args.pocket, args.player);

    for (auto [id, deck] : args.card_ids) {
        card_view *card = &m_context.cards.emplace(id);
        card->deck = deck;
        card->texture_back = card_textures::get().backfaces[enums::indexof(deck)];
        
        pocket.add_card(card);
    }
    for (card_view *card : pocket) {
        card->set_pos(pocket.get_pos() + pocket.get_offset(card));
    }
}

void game_scene::handle_game_update(UPD_TAG(remove_cards), const remove_cards_update &args) {
    for (auto *c : args.cards) {
        if (c == m_overlay) {
            m_overlay = nullptr;
        }
        if (c->pocket) {
            c->pocket->erase_card(c);
        }
        m_context.cards.erase(c->id);
    }
}

void game_scene::handle_game_update(UPD_TAG(move_card), const move_card_update &args) {
    add_animation<card_move_animation>(args.duration, [&]{
        pocket_view *old_pile = args.card->pocket;
        pocket_view *new_pile = &get_pocket(args.pocket, args.player);

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

        return anim;
    }());
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
    add_animation<cube_move_animation>(args.duration, [&]{
        auto &origin_pile = get_cube_pile(args.origin_card);
        auto &target_pile = get_cube_pile(args.target_card);

        cube_move_animation anim;
        for (int i=0; i<args.num_cubes; ++i) {
            auto &cube = target_pile.emplace_back(std::move(origin_pile.back()));
            origin_pile.pop_back();

            anim.add_cube(cube.get(), &target_pile);
        }
        return anim;
    }());
}

void game_scene::handle_game_update(UPD_TAG(move_train), const move_train_update &args) {
    add_animation<train_move_animation>(args.duration, &m_train, &m_stations, m_train_position = args.position);
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

        add_animation<card_flip_animation>(args.duration, args.card, false);
    }
}

void game_scene::handle_game_update(UPD_TAG(hide_card), const hide_card_update &args) {
    if (args.card->known) {
        args.card->known = false;
        add_animation<card_flip_animation>(args.duration, args.card, true);
    }
}

void game_scene::handle_game_update(UPD_TAG(tap_card), const tap_card_update &args) {
    if (args.card->inactive != args.inactive) {
        args.card->inactive = args.inactive;
        add_animation<card_tap_animation>(args.duration, args.card, args.inactive);
    }
}

void game_scene::handle_game_update(UPD_TAG(flash_card), const flash_card_update &args) {
    add_animation<card_flash_animation>(args.duration, args.card);
}

void game_scene::handle_game_update(UPD_TAG(short_pause), const short_pause_update &args) {
    add_animation<pause_animation>(args.duration, args.card);
}

void game_scene::move_player_views(anim_duration_type duration) {
    add_animation<player_move_animation>(duration, [&]{
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

        return anim;
    }());
}

void game_scene::handle_game_update(UPD_TAG(player_add), const player_add_update &args) {
    for (auto [player_id, user_id] : args.players) {
        player_view *p = &m_context.players.emplace(this, player_id, user_id);
        m_alive_players.push_back(p);
        
        p->m_role.texture_back = card_textures::get().backfaces[enums::indexof(card_deck_type::role)];
        if (user_id == parent->get_user_own_id()) {
            m_player_self = p;
        }

        p->set_user_info(parent->get_user_info(user_id));
    }

    if (m_player_self) {
        m_player_self->m_propic.set_border_color(widgets::propic_border_color);

#ifdef EDIT_GAME_PROPIC_ON_CLICK
        m_player_self->m_propic.set_onclick([&]{
            if (parent->browse_propic()) {
                parent->send_user_edit();
            }
        });
        m_player_self->m_propic.set_on_rightclick([&]{
            parent->reset_propic();
            parent->send_user_edit();
        });
#endif

        std::ranges::rotate(m_alive_players, std::ranges::find(m_alive_players, m_player_self));
    }
    
    move_player_views();
}

void game_scene::handle_game_update(UPD_TAG(player_order), const player_order_update &args) {
    player_view *first_player = m_alive_players.empty() ? nullptr : m_alive_players.front();
    m_alive_players = args.players;

    auto rotate_to = [&](player_view *p) {
        if (!p) return false;
        auto it = std::ranges::find(m_alive_players, p);
        if (it != m_alive_players.end()) {
            std::ranges::rotate(m_alive_players, it);
            return true;
        }
        return false;
    };

    rotate_to(m_player_self) || rotate_to(first_player);

    move_player_views(args.duration);
}

void game_scene::handle_message(SRV_TAG(lobby_add_user), const user_info_id_args &args) {
    auto it = std::ranges::find(m_context.players, args.user_id, &player_view::user_id);
    if (it != m_context.players.end()) {
        it->set_user_info(parent->get_user_info(args.user_id));
        it->set_position(it->get_position());
    }
}

void game_scene::handle_message(SRV_TAG(lobby_remove_user), const user_id_args &args) {
    auto it = std::ranges::find(m_context.players, args.user_id, &player_view::user_id);
    if (it != m_context.players.end()) {
        it->set_user_info(nullptr);
        it->set_position(it->get_position());
    }
}

void game_scene::handle_game_update(UPD_TAG(player_hp), const player_hp_update &args) {
    int prev_hp = args.player->hp;
    args.player->hp = args.hp;
    add_animation<player_hp_animation>(args.duration, args.player, prev_hp);
}

void game_scene::handle_game_update(UPD_TAG(player_gold), const player_gold_update &args) {
    args.player->set_gold(args.gold);
}

void game_scene::handle_game_update(UPD_TAG(player_show_role), const player_show_role_update &args) {
    role_card &card = args.player->m_role;
    if (card.role != args.role) {
        card.role = args.role;
        if (args.role == player_role::unknown) {
            card.known = false;
            add_animation<card_flip_animation>(args.duration, &card, true);
        } else {
            card.known = true;
            card.make_texture_front(parent->get_renderer());
            add_animation<card_flip_animation>(args.duration, &card, false);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(player_flags), const player_flags_update &args) {
    args.player->m_player_flags = args.flags;

    if (bool(args.flags & player_flags::removed)) {
        auto it = std::ranges::find(m_alive_players, args.player);
        if (it != m_alive_players.end()) {
            m_alive_players.erase(it);

            args.player->set_to_dead();

            args.player->set_position(args.player->m_role.get_pos() - sdl::point{(options.card_margin + widgets::profile_pic::size) / 2, 0});

            m_dead_players.push_back(args.player);
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(switch_turn), player_view *player) {
    if (player != m_playing) {
        m_playing = player;
        if (m_playing) {
            m_turn_border.emplace(m_playing, game_style::current_turn);
        } else {
            m_turn_border.reset();
        }
    }
}

void game_scene::handle_game_update(UPD_TAG(request_status), const request_status_args &args) {
    m_target.set_response_cards(args);

    if (args.status_text) {
        m_ui.set_status(format_game_string(args.status_text));
    }
}

void game_scene::handle_game_update(UPD_TAG(status_ready), const status_ready_args &args) {
    m_target.set_play_cards(args);
}

void game_scene::handle_game_update(UPD_TAG(game_flags), const game_flags &args) {
    m_game_flags = args;

    if (has_game_flags(game_flags::game_over)) {
        m_target.clear_status();
        m_ui.set_status(_("STATUS_GAME_OVER"));
        handle_message(SRV_TAG(lobby_owner){}, {parent->get_lobby_owner_id()});
    }
}

void game_scene::handle_game_update(UPD_TAG(play_sound), const std::string &sound_id) {
    play_sound(sound_id);
}

void game_scene::handle_game_update(UPD_TAG(status_clear)) {
    m_ui.clear_status();
    m_target.clear_status();
}