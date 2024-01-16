#include "lobby_list.h"

#include "../manager.h"
#include "net/options.h"

using namespace banggame;

lobby_line::lobby_line(lobby_list_scene *parent, const lobby_data &args)
    : parent(parent), lobby_id{args.lobby_id}
    , m_join_btn(_("BUTTON_JOIN"), [this]{ this->parent->do_join(lobby_id); })
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

lobby_list_scene::lobby_list_scene(client_manager *parent, const std::vector<lobby_data> &lobbies)
    : scene_base(parent)
    , m_make_lobby_btn(_("BUTTON_MAKE_LOBBY"), [this]{ do_make_lobby(); })
    , m_disconnect_btn(_("BUTTON_DISCONNECT"), [parent]{
        parent->disconnect();
    })
{
    m_lobby_name_box.set_value(parent->get_config().lobby_name);
    m_lobby_name_box.set_onenter([this](const std::string &value){ do_make_lobby(); });

    for (const lobby_data &lobby : lobbies) {
        m_lobby_lines.emplace_back(this, lobby);
    }
}

void lobby_list_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();
    
    m_disconnect_btn.set_rect(sdl::rect{20, 20, 100, 25});

    sdl::rect rect{100, 100, win_rect.w - 200, 25};
    for (auto &line : m_lobby_lines) {
        line.set_rect(rect);
        rect.y += 40;
    }

    m_lobby_name_box.set_rect(sdl::rect{100, rect.y, win_rect.w - 310, 25});
    m_make_lobby_btn.set_rect(sdl::rect{win_rect.w - 200, rect.y, 100, 25});
}

void lobby_list_scene::tick(duration_type time_elapsed) {
    m_lobby_name_box.tick(time_elapsed);
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
    parent->add_message<banggame::client_message_type::lobby_join>(lobby_id);
}

void lobby_list_scene::do_make_lobby() {
    if (m_lobby_name_box.get_value().empty()) {
        parent->add_chat_message(message_type::error, _("ERROR_NO_LOBBY_NAME"));
    } else {
        parent->get_config().lobby_name = m_lobby_name_box.get_value();
        parent->add_message<banggame::client_message_type::lobby_make>(m_lobby_name_box.get_value(), parent->get_config().options);
    }
}

void lobby_list_scene::handle_message(SRV_TAG(lobby_update), const lobby_data &args) {
    auto it = rn::find(m_lobby_lines, args.lobby_id, &lobby_line::lobby_id);
    if (it == m_lobby_lines.end()) {
        m_lobby_lines.emplace_back(this, args);
    } else {
        it->handle_update(args);
    }
    refresh_layout();
}

void lobby_list_scene::handle_message(SRV_TAG(lobby_removed), const lobby_id_args &args) {
    auto it = rn::find(m_lobby_lines, args.lobby_id, &lobby_line::lobby_id);
    if (it != m_lobby_lines.end()) {
        m_lobby_lines.erase(it);
    }
    refresh_layout();
}