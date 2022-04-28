#include "lobby_list.h"

#include "../manager.h"
#include "server/net_options.h"

lobby_line::lobby_line(lobby_list_scene *parent, const lobby_data &args)
    : parent(parent)
    , m_join_btn(_("BUTTON_JOIN"), [parent, lobby_id = args.lobby_id]{ parent->do_join(lobby_id); })
{
    handle_update(args);
}

void lobby_line::handle_update(const lobby_data &args) {
    m_name_text.set_value(args.name);
    m_players_text.set_value(fmt::format("{}/{}", args.num_players, banggame::lobby_max_players));
    m_state_text.set_value(_(args.state));
}

void lobby_line::set_rect(const sdl::rect &rect) {
    m_name_text.set_point(sdl::point{rect.x, rect.y});
    m_players_text.set_point(sdl::point{rect.x + rect.w - 250, rect.y});
    m_state_text.set_point(sdl::point{rect.x + rect.w - 200, rect.y});
    m_join_btn.set_rect(sdl::rect{rect.x + rect.w - 100, rect.y, 100, rect.h});
}

void lobby_line::render(sdl::renderer &renderer) {
    m_name_text.render(renderer);
    m_players_text.render(renderer);
    m_state_text.render(renderer);
    m_join_btn.render(renderer);
}

lobby_list_scene::lobby_list_scene(client_manager *parent)
    : scene_base(parent)
    , m_make_lobby_btn(_("BUTTON_MAKE_LOBBY"), [this]{ do_make_lobby(); })
    , m_disconnect_btn(_("BUTTON_DISCONNECT"), [parent]{
        parent->disconnect();
    })
{
    m_lobby_name_box.set_value(parent->get_config().lobby_name);
    m_lobby_name_box.set_onenter([this]{ do_make_lobby(); });
    parent->add_message<client_message_type::lobby_list>();
}

void lobby_list_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();
    
    m_disconnect_btn.set_rect(sdl::rect{20, 20, 100, 25});

    sdl::rect rect{100, 100, win_rect.w - 200, 25};
    for (auto &[id, line] : m_lobby_lines) {
        line.set_rect(rect);
        rect.y += 40;
    }

    m_lobby_name_box.set_rect(sdl::rect{100, rect.y, win_rect.w - 310, 25});
    m_make_lobby_btn.set_rect(sdl::rect{win_rect.w - 200, rect.y, 100, 25});
}

void lobby_list_scene::render(sdl::renderer &renderer) {
    for (auto &[id, line] : m_lobby_lines) {
        line.render(renderer);
    }

    m_lobby_name_box.render(renderer);
    m_make_lobby_btn.render(renderer);
    m_disconnect_btn.render(renderer);
}

void lobby_list_scene::do_join(int lobby_id) {
    parent->add_message<client_message_type::lobby_join>(lobby_id);
}

void lobby_list_scene::do_make_lobby() {
    if (m_lobby_name_box.get_value().empty()) {
        parent->add_chat_message(message_type::error, _("ERROR_NO_LOBBY_NAME"));
    } else {
        parent->get_config().lobby_name = m_lobby_name_box.get_value();
        parent->add_message<client_message_type::lobby_make>(m_lobby_name_box.get_value(), parent->get_config().expansions);
    }
}

void lobby_list_scene::handle_lobby_update(const lobby_data &args) {
    if (args.num_players == 0) {
        m_lobby_lines.erase(args.lobby_id);
    } else {
        auto [it, inserted] = m_lobby_lines.try_emplace(args.lobby_id, this, args);
        if (!inserted) {
            it->second.handle_update(args);
        }
    }
    refresh_layout();
}