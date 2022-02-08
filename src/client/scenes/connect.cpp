#include "connect.h"

#include "../manager.h"
#include "../tinyfd/tinyfiledialogs.h"

#include "game/sizes.h"
#include "utils/resource.h"

DECLARE_RESOURCE(icon_default_user_png)

recent_server_line::recent_server_line(connect_scene *parent, const std::string &address)
    : parent(parent)
    , m_address_text(address)
    , m_connect_btn(_("BUTTON_CONNECT"), [parent, &address]{
        parent->do_connect(address);
    })
    , m_delete_btn(_("BUTTON_DELETE"), [this]{
        this->parent->do_delete_address(this);
    }) {}

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

connect_scene::connect_scene(game_manager *parent)
    : scene_base(parent)
    , m_username_label(_("LABEL_USERNAME"))
    , m_address_label(_("LABEL_NEW_ADDRESS"))
    , m_connect_btn(_("BUTTON_CONNECT"), [this]{
        do_connect(m_address_box.get_value());
    })
    , m_create_server_btn(_("BUTTON_CREATE_SERVER"), [this]{
        do_create_server();
    })
    , m_error_text(sdl::text_style{
        .text_color = sdl::rgb(sdl::error_text_rgb),
        .text_ptsize = sdl::error_text_ptsize
    })
{
    m_username_box.set_value(parent->get_config().user_name);
    m_address_box.set_onenter([this]{
        do_connect(m_address_box.get_value());
    });
    for (const auto &obj : parent->get_config().recent_servers) {
        m_recents.emplace_back(this, obj);
    }

    if (parent->get_config().profile_image_data.empty()) {
        m_propic = sdl::surface(GET_RESOURCE(icon_default_user_png));
    } else {
        m_propic = decode_profile_image(parent->get_config().profile_image_data);
    }
}

void connect_scene::resize(int width, int height) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    
    auto propic_rect = m_propic.get_rect();
    m_propic_pos.x = 115 + label_rect.w + (sizes::propic_size - propic_rect.w) / 2;
    m_propic_pos.y = m_username_box.get_rect().y + (m_username_box.get_rect().h - propic_rect.h) / 2;

    m_username_box.set_rect(sdl::rect{120 + sizes::propic_size + label_rect.w, 50, width - 220 - sizes::propic_size - label_rect.w, 25});

    m_create_server_btn.set_rect(sdl::rect{(width - 200) / 2, 100, 200, 25});

    sdl::rect rect{100, 150, width - 200, 25};
    for (auto &line : m_recents) {
        line.set_rect(rect);
        rect.y += 50;
    }

    label_rect = m_address_label.get_rect();
    label_rect.x = rect.x;
    label_rect.y = rect.y;
    m_address_label.set_rect(label_rect);

    m_address_box.set_rect(sdl::rect{rect.x + label_rect.w + 10, rect.y, rect.w - 120 - label_rect.w, rect.h});
    
    m_connect_btn.set_rect(sdl::rect{rect.x + rect.w - 100, rect.y, 100, rect.h});

    m_error_text.set_point(sdl::point{(width - m_error_text.get_rect().w) / 2, rect.y + 50});
}

void connect_scene::render(sdl::renderer &renderer) {
    m_username_label.render(renderer);
    m_username_box.render(renderer);

    m_propic.render(renderer, m_propic_pos);

    for (auto &line : m_recents) {
        line.render(renderer);
    }

    m_address_label.render(renderer);
    m_address_box.render(renderer);
    m_connect_btn.render(renderer);
    m_create_server_btn.render(renderer);
    m_error_text.render(renderer);
}

void connect_scene::handle_event(const sdl::event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        sdl::point mouse_pt{event.button.x, event.button.y};
        sdl::rect profile_rect = m_propic.get_rect();
        profile_rect.x = m_propic_pos.x;
        profile_rect.y = m_propic_pos.y;
        if (mouse_pt.x >= profile_rect.x && mouse_pt.x <= profile_rect.x + profile_rect.w
            && mouse_pt.y >= profile_rect.y && mouse_pt.y <= profile_rect.y + profile_rect.h) {
            do_browse_propic();
        }
    }
}

void connect_scene::show_error(const std::string &error) {
    int y = m_error_text.get_rect().y;
    m_error_text.redraw(error);
    m_error_text.set_point(sdl::point{(parent->width() - m_error_text.get_rect().w)/ 2, y});
}

void connect_scene::do_connect(const std::string &address) {
    if (m_username_box.get_value().empty()) {
        show_error(_("ERROR_NO_USERNAME"));
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

    resize(parent->width(), parent->height());
}

void connect_scene::do_browse_propic() {
    const char *filters[] = {"*.jpg", "*.png"};
    auto &cfg = parent->get_config();
    const char *ret = tinyfd_openFileDialog(_("BANG_TITLE").c_str(), cfg.profile_image.c_str(), 2, filters, _("DIALOG_IMAGE_FILES").c_str(), 0);
    if (ret) {
        try {
            cfg.profile_image_data = encode_profile_image(sdl::surface(resource(ret)));
            m_propic = decode_profile_image(cfg.profile_image_data);
            cfg.profile_image.assign(ret);
            resize(parent->width(), parent->height());
        } catch (const std::runtime_error &e) {
            show_error(e.what());
        }
    }
}

void connect_scene::do_create_server() {
    if (parent->start_listenserver()) {
        do_connect("localhost");
    }
}