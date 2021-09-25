#include "button.h"

using namespace sdl;

button::button(const std::string &label, button_callback_fun &&onclick, const button_style &style)
    : m_style(style)
    , m_text(label, style)
    , m_onclick(std::move(onclick)) {}
    
void button::render(renderer &renderer) {
    switch (m_state) {
    case state_up: renderer.set_draw_color(m_style.up_color); break;
    case state_hover: renderer.set_draw_color(m_style.hover_color); break;
    case state_down: renderer.set_draw_color(m_style.down_color); break;
    }
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    SDL_Point pos = m_text_pos;
    if (m_state == state_down) {
        ++pos.x;
        ++pos.y;
    }

    m_text.set_point(pos);
    m_text.render(renderer);
}

void button::handle_event(const SDL_Event &event) {
    auto pt_in_button = [&](int x, int y) {
        return x >= m_border_rect.x && x <= (m_border_rect.x + m_border_rect.w)
            && y >= m_border_rect.y && y <= (m_border_rect.y + m_border_rect.h);
    };

    switch (event.type) {
    case SDL_MOUSEMOTION:
        if (pt_in_button(event.motion.x, event.motion.y)) {
            if (m_state == state_up) {
                m_state = state_hover;
            }
        } else {
            if (m_state == state_hover) {
                m_state = state_up;
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT
            && pt_in_button(event.button.x, event.button.y)) {
            m_state = state_down;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT
            && m_state == state_down) {
            if (pt_in_button(event.button.x, event.button.y)) {
                if (m_onclick) m_onclick();
                m_state = state_hover;
            } else {
                m_state = state_up;
            }
        }
        break;
    };
}