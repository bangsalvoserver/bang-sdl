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

    if (focused() && (m_ticks++ % 50) < 25) {
        SDL_RenderDrawLine(renderer.get(), linex, m_border_rect.y + m_style.margin, linex, m_border_rect.y + m_border_rect.h - m_style.margin);
    }
}

void textbox::on_gain_focus() {
    m_ticks = 0;
    SDL_StartTextInput();
}

void textbox::on_lose_focus() {
    SDL_StopTextInput();
}

bool textbox::handle_event(const event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT && point_in_rect(point{event.button.x, event.button.y}, m_border_rect)) {
            set_focus(this);
            return true;
        }
        return false;
    case SDL_KEYDOWN:
        if (focused()) {
            switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (!m_value.empty()) {
                    m_value.resize(m_value.size() - 1);
                }
                update_texture();
                return true;
            case SDLK_RETURN:
                if (on_enter) {
                    on_enter();
                    return true;
                }
            }
        }
        return false;
    case SDL_TEXTINPUT:
        if (focused() && event.text.text[0] != '\n') {
            m_value.append(event.text.text);
            update_texture();
            return true;
        }
        return false;
    default:
        return false;
    }
}