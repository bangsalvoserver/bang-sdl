#include "textbox.h"

using namespace sdl;

textbox::textbox(const textbox_style &style) : m_style (style) {}

void textbox::render(renderer &renderer) {
    renderer.set_draw_color(m_style.background_color);
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    int linex = m_border_rect.x + m_style.margin;

    if (m_tex) {
        m_tex.set_point(point{m_border_rect.x + m_style.margin, m_border_rect.y + m_style.margin});
        m_tex.render(renderer);

        linex = m_tex.get_rect().x + m_tex.get_rect().w;
    }

    if (m_active) {
        SDL_RenderDrawLine(renderer.get(), linex, m_border_rect.y + m_style.margin, linex, m_border_rect.y + m_border_rect.h - m_style.margin);
    }
}

void textbox::handle_event(const event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        m_active = event.button.button == SDL_BUTTON_LEFT && point_in_rect(point{event.button.x, event.button.y}, m_border_rect);
        break;
    case SDL_KEYDOWN:
        if (m_active && event.key.keysym.sym == SDLK_BACKSPACE) {
            if (!m_value.empty()) {
                m_value.resize(m_value.size() - 1);
            }
            update_texture();
        }
        break;
    case SDL_TEXTINPUT:
        if (m_active && event.text.text[0] != '\n') {
            m_value.append(event.text.text);
            update_texture();
        }
        break;
    }
}