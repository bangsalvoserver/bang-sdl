#include "connect.h"

#include "../manager.h"

connect_scene::connect_scene(game_manager *parent)
    : scene_base(parent)
    , m_connect_btn("Connetti", [this]{
        this->parent->connect(m_address_box.get_value());
    })
    , m_error_text(sdl::text_style{
        {0xff, 0x0, 0x0, 0xff},
        sdl::default_text_style.text_font,
        20
    }) {}

void connect_scene::render(sdl::renderer &renderer) {
    m_address_box.set_rect(sdl::rect{115, 100, 300, 25});
    m_address_box.render(renderer);

    m_connect_btn.set_rect(sdl::rect{425, 100, 100, 25});
    m_connect_btn.render(renderer);

    m_error_text.set_point(sdl::point{(m_width - m_error_text.get_rect().w) / 2, 150});
    m_error_text.render(renderer);
}

void connect_scene::handle_event(const sdl::event &event) {
    m_address_box.handle_event(event);
    m_connect_btn.handle_event(event);
}

void connect_scene::show_error(const std::string &error) {
    m_error_text.redraw(error);
}