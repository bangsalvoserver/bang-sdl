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
    , m_connect_btn("Connetti", [this]{
        do_connect(m_address_box.get_value());
    })
    , m_error_text(sdl::text_style{
        {0xff, 0x0, 0x0, 0xff},
        sdl::default_text_style.text_font,
        20
    })
{
    m_address_box.set_onenter([this]{
        do_connect(m_address_box.get_value());
    });
    for (const auto &obj : parent->get_config().recent_servers) {
        m_recents.emplace_back(this, obj);
    }
}

void connect_scene::render(sdl::renderer &renderer) {
    sdl::rect rect{100, 50, parent->width() - 200, 25};
    for (auto &line : m_recents) {
        line.set_rect(rect);
        line.render(renderer);

        rect.y += 50;
    }

    m_address_box.set_rect(sdl::rect{rect.x, rect.y, rect.w - 110, rect.h});
    m_address_box.render(renderer);

    m_connect_btn.set_rect(sdl::rect{rect.x + rect.w - 100, rect.y, 100, rect.h});
    m_connect_btn.render(renderer);

    m_error_text.set_point(sdl::point{(parent->width() - m_error_text.get_rect().w) / 2, rect.y + 50});
    m_error_text.render(renderer);
}

void connect_scene::show_error(const std::string &error) {
    m_error_text.redraw(error);
}

void connect_scene::do_connect(const std::string &address) {
    parent->connect(address);
}