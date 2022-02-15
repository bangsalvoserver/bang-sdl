#include "textbox.h"

using namespace widgets;

textbox::textbox(const textbox_style &style) : m_style (style) {}

void textbox::render(sdl::renderer &renderer) {
    renderer.set_draw_color(m_style.background_color);
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    int linex = m_border_rect.x + m_style.margin;

    if (m_tex) {
        m_tex.set_point(sdl::point{m_border_rect.x + m_style.margin, m_border_rect.y + m_style.margin});
        m_tex.render_cropped(renderer, sdl::rect{
            m_border_rect.x + m_style.margin,
            m_border_rect.y + m_style.margin,
            m_border_rect.w - 2 * m_style.margin,
            m_border_rect.h
        });

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

void pop_back_utf8(std::string& utf8) {
    if (utf8.empty()) return;

    auto cp = utf8.data() + utf8.size();
    while (--cp >= utf8.data() && ((*cp & 0b10000000) && !(*cp & 0b01000000))) {}
    if (cp >= utf8.data())
        utf8.resize(cp - utf8.data());
}

bool textbox::handle_event(const sdl::event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT && sdl::point_in_rect(sdl::point{event.button.x, event.button.y}, m_border_rect)) {
            set_focus(this);
            return true;
        }
        return false;
    case SDL_KEYDOWN:
        if (focused()) {
            switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                pop_back_utf8(m_value);
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