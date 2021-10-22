#include "connect.h"

#include "../manager.h"

recent_server_line::recent_server_line(connect_scene *parent, const std::string &address)
    : parent(parent)
    , m_address_text(address)
    , m_connect_btn("Connetti", [parent, &address]{
        parent->do_connect(address);
    }) {}

void recent_server_line::set_rect(const sdl::rect &rect) {
    m_address_text.set_point(sdl::point(rect.x, rect.y));
    m_connect_btn.set_rect(sdl::rect(rect.x + rect.w - 100, rect.y, 100, rect.h));
}

void recent_server_line::render(sdl::renderer &renderer) {
    m_address_text.render(renderer);
    m_connect_btn.render(renderer);
}

connect_scene::connect_scene(game_manager *parent)
    : scene_base(parent)
    , m_username_label("Nome utente:")
    , m_address_label("Nuovo indirizzo:")
    , m_connect_btn("Connetti", [this]{
        do_connect(m_address_box.get_value());
    })
    , m_error_text(sdl::text_style{
        {0xff, 0x0, 0x0, 0xff},
        sdl::default_text_style.text_font,
        20
    })
{
    m_username_box.set_value(parent->get_config().user_name);
    m_address_box.set_onenter([this]{
        do_connect(m_address_box.get_value());
    });
    for (const auto &obj : parent->get_config().recent_servers) {
        m_recents.emplace_back(this, obj);
    }
}

void connect_scene::resize(int width, int height) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    
    m_username_box.set_rect(sdl::rect{100 + label_rect.w + 10, 50, width - 210 - label_rect.w, 25});

    sdl::rect rect{100, 100, width - 200, 25};
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

    for (auto &line : m_recents) {
        line.render(renderer);
    }

    m_address_label.render(renderer);
    m_address_box.render(renderer);
    m_connect_btn.render(renderer);
    m_error_text.render(renderer);
}

void connect_scene::show_error(const std::string &error) {
    int y = m_error_text.get_rect().y;
    m_error_text.redraw(error);
    m_error_text.set_point(sdl::point{(parent->width() - m_error_text.get_rect().w)/ 2, y});
}

void connect_scene::do_connect(const std::string &address) {
    if (m_username_box.get_value().empty()) {
        show_error("Specificare un nome utente");
    } else {
        parent->get_config().user_name = m_username_box.get_value();
        parent->connect(address);
    }
}