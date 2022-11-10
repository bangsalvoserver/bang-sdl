#include "lobby.h"

#include "../manager.h"
#include "../media_pak.h"

#include "lobby_list.h"

using namespace banggame;

lobby_scene::lobby_player_item::lobby_player_item(lobby_scene *parent, int id, const user_info &args)
    : parent(parent)
    , m_user_id(id)
    , m_name_text(args.name, widgets::text_style{
        .text_font = &media_pak::font_bkant_bold
    })
    , m_propic(args.profile_image) {}

void lobby_scene::lobby_player_item::set_pos(int x, int y) {
    m_propic.set_pos(sdl::point{
        x + widgets::profile_pic::size / 2,
        y + widgets::profile_pic::size / 2
    });

    m_name_text.set_point(sdl::point{
        x + widgets::profile_pic::size + 10,
        y + (widgets::profile_pic::size - m_name_text.get_rect().h) / 2});
}

void lobby_scene::lobby_player_item::render(sdl::renderer &renderer) {
    if (parent->parent->get_user_own_id() == m_user_id) {
        m_propic.set_border_color(widgets::propic_border_color);
    } else {
        m_propic.set_border_color({});
    }
    m_propic.render(renderer);
    m_name_text.render(renderer);

    if (parent->parent->get_lobby_owner_id() == m_user_id) {
        sdl::rect rect = media_pak::get().icon_owner.get_rect();
        rect.x = m_propic.get_pos().x - 60;
        rect.y = m_propic.get_pos().y - rect.h / 2;
        media_pak::get().icon_owner.render_colored(renderer, rect, widgets::propic_border_color);
    }
}

lobby_scene::expansion_box::expansion_box(const std::string &label, banggame::card_expansion_type flag, banggame::card_expansion_type check)
    : widgets::checkbox(label, widgets::button_style{
        .text = {
            .text_font = &media_pak::font_perdido
        }
    })
    , m_flag(flag)
{
    set_value(bool(flag & check));
}

lobby_scene::lobby_scene(client_manager *parent, const lobby_info &args)
    : scene_base(parent)
    , m_lobby_name_text(args.name, widgets::text_style {
        .text_font = &media_pak::font_bkant_bold
    })
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{ parent->add_message<banggame::client_message_type::lobby_leave>(); })
    , m_start_btn(_("BUTTON_START"), [parent]{ parent->add_message<banggame::client_message_type::game_start>(); })
    , m_chat_btn(_("BUTTON_CHAT"), [parent]{ parent->enable_chat(); })
{
    for (auto E : enums::enum_values_v<banggame::card_expansion_type>) {
        if (!parent->get_config().allow_unofficial_expansions && bool(banggame::unofficial_expansions & E)) continue;

        m_checkboxes.emplace_back(_(E), E, args.options.expansions)
            .set_ontoggle([this]{ send_lobby_edited(); });
    }
}

void lobby_scene::handle_message(SRV_TAG(lobby_edited), const lobby_info &info) {
    m_lobby_name_text.set_value(info.name);
    
    for (auto &checkbox : m_checkboxes) {
        checkbox.set_value(bool(info.options.expansions & checkbox.m_flag));
    }

    if (parent->get_lobby_owner_id() == parent->get_user_own_id()) {
        parent->get_config().options = info.options;
    }
}

void lobby_scene::send_lobby_edited() {
    auto &options = parent->get_config().options;
    options.expansions = {};
    for (const auto &box : m_checkboxes) {
        if (box.get_value()) {
            options.expansions |= box.m_flag;
        }
    }
    parent->add_message<banggame::client_message_type::lobby_edit>(m_lobby_name_text.get_value(), options);
}

void lobby_scene::handle_message(SRV_TAG(lobby_owner), const user_id_args &args) {
    for (auto &checkbox : m_checkboxes) {
        checkbox.set_locked(args.user_id != parent->get_user_own_id());
    }

    m_start_btn.set_enabled(args.user_id == parent->get_user_own_id());
}

void lobby_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();
    
    sdl::rect checkbox_rect{130, 60, 0, 25};
    for (auto &box : m_checkboxes) {
        box.set_rect(checkbox_rect);
        checkbox_rect.y += 50;
    }

    m_lobby_name_text.set_point(sdl::point{win_rect.w / 2 + 5, 20});

    int y = 60;
    for (auto &line : m_player_list) {
        line.set_pos(win_rect.w / 2, y);
        y += widgets::profile_pic::size + 10;
    }

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_start_btn.set_rect(sdl::rect{130, 20, 100, 25});
    m_chat_btn.set_rect(sdl::rect{win_rect.w - 120, win_rect.h - 40, 100, 25});
}

void lobby_scene::render(sdl::renderer &renderer) {
    m_lobby_name_text.render(renderer);

    for (auto &line : m_player_list) {
        line.render(renderer);
    }

    m_start_btn.render(renderer);
    
    for (auto &box : m_checkboxes) {
        box.render(renderer);
    }

    m_leave_btn.render(renderer);

    m_chat_btn.render(renderer);
}

void lobby_scene::handle_message(SRV_TAG(lobby_add_user), const lobby_add_user_args &args) {
    if (const user_info *user = parent->get_user_info(args.user_id)) {
        m_player_list.emplace_back(this, args.user_id, *user);
        refresh_layout();
    }
}

void lobby_scene::handle_message(SRV_TAG(lobby_remove_user), const user_id_args &args) {
    auto it = std::ranges::find(m_player_list, args.user_id, &lobby_player_item::user_id);
    if (it != m_player_list.end()) {
        m_player_list.erase(it);
    }

    refresh_layout();
}