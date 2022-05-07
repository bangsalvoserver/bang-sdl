#include "connect.h"

#include "../manager.h"
#include "../media_pak.h"
#include "../os_api.h"

recent_server_line::recent_server_line(connect_scene *parent, const std::string &address)
    : parent(parent)
    , m_address_text(address)
    , m_connect_btn(_("BUTTON_CONNECT"), [this]{
        this->parent->do_connect(m_address_text.get_value());
    })
    , m_delete_btn(_("BUTTON_DELETE"), [this]{ this->parent->do_delete_address(this); }) {}

void recent_server_line::set_rect(const sdl::rect &rect) {
    m_address_text.set_point(sdl::point(rect.x, rect.y));
    m_connect_btn.set_rect(sdl::rect(rect.x + rect.w - 100, rect.y, 100, rect.h));
    m_delete_btn.set_rect(sdl::rect(rect.x + rect.w - 210, rect.y, 100, rect.h));
}

void recent_server_line::render(sdl::renderer &renderer) {
    m_address_text.render(renderer);
    m_connect_btn.render(renderer);
    m_delete_btn.render(renderer);
}

connect_scene::connect_scene(client_manager *parent)
    : scene_base(parent)
    , m_username_label(_("LABEL_USERNAME"))
    , m_address_label(_("LABEL_NEW_ADDRESS"))
    , m_connect_btn(_("BUTTON_CONNECT"), [this]{
        do_connect(m_address_box.get_value());
    })
    , m_create_server_btn(_("BUTTON_CREATE_SERVER"), [this]{ do_create_server(); })
{
    m_username_box.set_value(parent->get_config().user_name);
    m_address_box.set_onenter([this]{
        do_connect(m_address_box.get_value());
    });
    for (const auto &obj : parent->get_config().recent_servers) {
        m_recents.emplace_back(this, obj);
    }

    m_propic.set_onclick([this]{ do_browse_propic(); });
    m_propic.set_on_rightclick([this]{ reset_propic(); });
    m_propic.set_texture(parent->get_config().profile_image_data);
}

void connect_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();

    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    
    m_username_box.set_rect(sdl::rect{
        125 + label_rect.w + widgets::profile_pic::size,
        50,
        win_rect.w - 225 - widgets::profile_pic::size - label_rect.w,
        25});

    m_propic.set_pos(sdl::point{
        115 + label_rect.w + widgets::profile_pic::size / 2,
        m_username_box.get_rect().y + m_username_box.get_rect().h / 2
    });

    m_create_server_btn.set_rect(sdl::rect{(win_rect.w - 200) / 2, 100, 200, 25});

    sdl::rect rect{100, 150, win_rect.w - 200, 25};
    for (auto &line : m_recents) {
        line.set_rect(rect);
        rect.y += 35;
    }

    label_rect = m_address_label.get_rect();
    label_rect.x = rect.x;
    label_rect.y = rect.y;
    m_address_label.set_rect(label_rect);

    m_address_box.set_rect(sdl::rect{rect.x + label_rect.w + 15, rect.y, rect.w - 125 - label_rect.w, rect.h});
    
    m_connect_btn.set_rect(sdl::rect{rect.x + rect.w - 100, rect.y, 100, rect.h});
}

void connect_scene::render(sdl::renderer &renderer) {
    m_username_label.render(renderer);
    m_username_box.render(renderer);
    m_propic.render(renderer);

    for (auto &line : m_recents) {
        line.render(renderer);
    }

    m_address_label.render(renderer);
    m_address_box.render(renderer);
    m_connect_btn.render(renderer);
    m_create_server_btn.render(renderer);
}

void connect_scene::do_connect(const std::string &address) {
    if (m_username_box.get_value().empty()) {
        parent->add_chat_message(message_type::error, _("ERROR_NO_USERNAME"));
    } else {
        parent->get_config().user_name = m_username_box.get_value();
        parent->connect(address);
    }
}

void connect_scene::do_delete_address(recent_server_line *addr) {
    auto it = std::ranges::find(m_recents, addr, [](const auto &obj) { return &obj; });
    auto &servers = parent->get_config().recent_servers;
    servers.erase(servers.begin() + std::distance(m_recents.begin(), it));
    m_recents.erase(it);

    refresh_layout();
}

void connect_scene::do_browse_propic() {
    auto &cfg = parent->get_config();
    if (auto value = os_api::open_file_dialog(
            _("BANG_TITLE"),
            cfg.profile_image,
            {
                {{"*.jpg","*.jpeg","*.png"}, _("DIALOG_IMAGE_FILES")},
                {{"*.*"}, _("DIALOG_ALL_FILES")}
            },
            &parent->get_window()
        )) {
        try {
            cfg.profile_image_data = widgets::profile_pic::scale_profile_image(sdl::surface(resource(*value)));
            m_propic.set_texture(cfg.profile_image_data);
            cfg.profile_image = value->string();
            refresh_layout();
        } catch (const std::runtime_error &e) {
            parent->add_chat_message(message_type::error, e.what());
        }
    }
}

void connect_scene::reset_propic() {
    m_propic.set_texture(nullptr);
    parent->get_config().profile_image.clear();
    parent->get_config().profile_image_data.reset();
    refresh_layout();
}

void connect_scene::do_create_server() {
    if (m_username_box.get_value().empty()) {
        parent->add_chat_message(message_type::error, _("ERROR_NO_USERNAME"));
    } else if (parent->start_listenserver()) {
        do_connect("localhost");
    } else {
        parent->add_chat_message(message_type::error, _("ERROR_STARTING_LISTENSERVER"));
    }
}