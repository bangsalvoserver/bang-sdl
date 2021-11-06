#include "button.h"

using namespace sdl;

button::button(const std::string &label, button_callback_fun &&onclick, const button_style &style)
    : m_style(style)
    , m_text(label, style)
    , m_onclick(std::move(onclick)) {}

void button::render(renderer &renderer) {
    if (!enabled()) return;
    
    renderer.set_draw_color([&]{
        if (toggled) return m_style.down_color;
        switch (m_state) {
        case state_hover:   return m_style.hover_color;
        case state_down:    return m_style.down_color;
        case state_up:
        default:            return m_style.up_color;
        }
    }());
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    point pos = m_text_pos;
    if (m_state == state_down) {
        ++pos.x;
        ++pos.y;
    }

    m_text.set_point(pos);
    m_text.render(renderer);
}

bool button::handle_event(const event &event) {
    switch (event.type) {
    case SDL_MOUSEMOTION:
        if (point_in_rect(point{event.motion.x, event.motion.y}, m_border_rect)) {
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
            && point_in_rect(point{event.motion.x, event.motion.y}, m_border_rect)) {
            m_state = state_down;
            set_focus(this);
            return true;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT
            && m_state == state_down) {
            if (point_in_rect(point{event.motion.x, event.motion.y}, m_border_rect)) {
                m_state = state_hover;
                if (m_onclick) {
                    m_onclick();
                    return true;
                }
            } else {
                m_state = state_up;
            }
        }
        break;
    };
    return false;
}