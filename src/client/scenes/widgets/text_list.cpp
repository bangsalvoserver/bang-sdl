#include "text_list.h"

#include <ranges>

namespace sdl {

    void text_list::set_rect(const rect &new_rect) {
        m_rect = new_rect;

        int y = m_rect.y + m_rect.h;
        for (stattext &obj : m_messages | std::views::reverse) {
            rect text_rect = obj.get_rect();
            text_rect.x = m_rect.x;
            switch (m_style.align) {
            case text_alignment::left: break;
            case text_alignment::middle:
                text_rect.x += (m_rect.w - text_rect.w) / 2; break;
            case text_alignment::right:
                text_rect.x += m_rect.w - text_rect.w; break;
            }
            text_rect.y = y - text_rect.h;
            obj.set_rect(text_rect);

            y -= m_style.text_offset;
        }

        while (m_messages.front().get_rect().y + m_messages.front().get_rect().h < get_rect().y) {
            m_messages.pop_front();
        }
    }

    void text_list::render(renderer &renderer) {
        for (auto &obj : m_messages) {
            obj.render(renderer);
        }
    }

    void text_list::add_message(const std::string &message) {
        m_messages.emplace_back(message, m_style.text);
        set_rect(m_rect);
    }

}