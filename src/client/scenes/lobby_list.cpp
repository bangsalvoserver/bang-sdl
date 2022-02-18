#include "lobby_list.h"

#include "../manager.h"
#include "server/net_options.h"

lobby_line::lobby_line(lobby_list_scene *parent, const lobby_data &args)
    : parent(parent)
    , m_id(args.lobby_id)
    , m_name_text(args.name)
    , m_players_text(std::to_string(args.num_players) + '/' + std::to_string(banggame::lobby_max_players))
    , m_state_text(_(args.state))
    , m_join_btn(_("BUTTON_JOIN"), std::bind(&lobby_list_scene::do_join, parent, args.lobby_id)) {}

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

lobby_list_scene::lobby_list_scene(game_manager *parent)
    : scene_base(parent)
    , m_make_lobby_btn(_("BUTTON_MAKE_LOBBY"), std::bind(&lobby_list_scene::do_make_lobby, this))
    , m_disconnect_btn(_("BUTTON_DISCONNECT"), std::bind(&game_manager::disconnect, parent))
{
    m_lobby_name_box.set_value(parent->get_config().lobby_name);
    m_lobby_name_box.set_onenter(std::bind(&lobby_list_scene::do_make_lobby, this));
    parent->add_message<client_message_type::lobby_list>();
}

void lobby_list_scene::resize(int width, int height) {
    m_disconnect_btn.set_rect(sdl::rect{20, 20, 100, 25});

    sdl::rect rect{100, 100, width - 200, 25};
    for (auto &line : m_lobby_lines) {
        line.set_rect(rect);
        rect.y += 40;
    }

    m_lobby_name_box.set_rect(sdl::rect{100, rect.y, width - 310, 25});
    m_make_lobby_btn.set_rect(sdl::rect{width - 200, rect.y, 100, 25});
}

void lobby_list_scene::render(sdl::renderer &renderer) {
    for (auto &line : m_lobby_lines) {
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
    if (!m_lobby_name_box.get_value().empty()) {
        parent->get_config().lobby_name = m_lobby_name_box.get_value();
        parent->add_message<client_message_type::lobby_make>(m_lobby_name_box.get_value(), parent->get_config().expansions);
    }
}

void lobby_list_scene::set_lobby_list(const std::vector<lobby_data> &args) {
    m_lobby_lines.clear();
    for (const auto &line : args) {
        m_lobby_lines.emplace_back(this, line);
    }

    resize(parent->width(), parent->height());
}

void lobby_list_scene::handle_lobby_update(const lobby_data &args) {
    auto it = std::ranges::find(m_lobby_lines, args.lobby_id, &lobby_line::id);
    if (it != m_lobby_lines.end()) {
        if (args.num_players == 0) {
            m_lobby_lines.erase(it);
        } else {
            *it = lobby_line(this, args);
        }
    } else {
        m_lobby_lines.emplace_back(this, args);
    }
    resize(parent->width(), parent->height());
}