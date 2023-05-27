#include "lobby.h"

#include "../manager.h"
#include "../media_pak.h"

using namespace banggame;

lobby_scene::lobby_player_item::lobby_player_item(lobby_scene *parent, int id, const banggame::user_info &args)
    : parent(parent)
    , m_user_id(id)
    , m_name_text(args.name, widgets::text_style{
        .text_font = &media_pak::font_bkant_bold
    })
    , m_propic(sdl::texture(parent->parent->get_renderer(), sdl::image_pixels_to_surface(args.profile_image)))
{
    client_manager *mgr = parent->parent;
    if (id == mgr->get_user_own_id()) {
        m_propic.set_onclick([mgr]{
            if (auto tex = mgr->browse_propic()) {
                mgr->send_user_edit();
            }
        });
        m_propic.set_on_rightclick([mgr]{
            mgr->reset_propic();
            mgr->send_user_edit();
        });
    }
}

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

using box_vector = std::vector<std::unique_ptr<option_input_box_base>>;

template<size_t I>
static void add_box(box_vector &vector, lobby_scene *parent, banggame::game_options &options) {
    auto field_data = reflector::get_field_data<I>(options);
    auto &field = field_data.get();
    using box_type = option_input_box<std::remove_reference_t<decltype(field)>>;
    if constexpr (requires (std::string label) { box_type{parent, label, field}; }) {
        vector.emplace_back(std::make_unique<box_type>(parent,
            _(fmt::format("game_options::{}", field_data.name())), field));
    }
}

template<size_t ... Is>
static void add_boxes(box_vector &vector, lobby_scene *parent, banggame::game_options &options, std::index_sequence<Is...>) {
    (add_box<Is>(vector, parent, options), ...);
}

lobby_scene::lobby_scene(client_manager *parent, const lobby_info &args)
    : scene_base(parent)
    , m_lobby_name_text(args.name, widgets::text_style {
        .text_font = &media_pak::font_bkant_bold
    })
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{ parent->add_message<banggame::client_message_type::lobby_leave>(); })
    , m_start_btn(_("BUTTON_START"), [parent]{ parent->add_message<banggame::client_message_type::game_start>(); })
    , m_chat_btn(_("BUTTON_CHAT"), [parent]{ parent->enable_chat(); })
    , m_lobby_options(args.options)
{
    add_boxes(m_option_boxes, this, m_lobby_options, std::make_index_sequence<reflector::num_fields<banggame::game_options>>());
}

void lobby_scene::handle_message(SRV_TAG(lobby_edited), const lobby_info &info) {
    m_lobby_name_text.set_value(info.name);

    m_lobby_options = info.options;

    if (parent->get_lobby_owner_id() == parent->get_user_own_id()) {
        parent->get_config().options = m_lobby_options;
    }

    for (auto &box : m_option_boxes) {
        box->update_value();
    }
}

void lobby_scene::send_lobby_edited() {
    parent->add_message<banggame::client_message_type::lobby_edit>(m_lobby_name_text.get_value(), m_lobby_options);
    parent->get_config().options = m_lobby_options;
}

void lobby_scene::handle_message(SRV_TAG(lobby_owner), const user_id_args &args) {
    for (auto &box : m_option_boxes) {
        box->set_locked(args.user_id != parent->get_user_own_id());
    }

    m_start_btn.set_enabled(args.user_id == parent->get_user_own_id());
}

void lobby_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();

    sdl::point pt{130, 60};
    for (auto &box : m_option_boxes) {
        box->set_pos(pt);
        
        auto rect = box->get_rect();
        pt.y = rect.y + rect.h + 10;
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

    for (auto &box : m_option_boxes) {
        box->render(renderer);
    }

    m_leave_btn.render(renderer);

    m_chat_btn.render(renderer);
}

void lobby_scene::handle_event(const sdl::event &event) {
    switch (event.type) {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_RETURN) {
            parent->enable_chat();
        }
        break;
    }
}

void lobby_scene::handle_message(SRV_TAG(lobby_add_user), const user_info_id_args &args) {
    if (const banggame::user_info *user = parent->get_user_info(args.user_id)) {
        if (auto it = std::ranges::find(m_player_list, args.user_id, &lobby_player_item::user_id); it != m_player_list.end()) {
            *it = lobby_player_item(this, args.user_id, *user);
        } else {
            m_player_list.emplace_back(this, args.user_id, *user);
        }
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