#include "text_list.h"

#include <ranges>

namespace widgets {

    void text_list::set_rect(const sdl::rect &new_rect) {
        m_rect = new_rect;

        int y = m_rect.y + m_rect.h;
        for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
            sdl::rect text_rect = it->get_rect();
            text_rect.x = m_rect.x;
            text_rect.y = y - text_rect.h;
            if (text_rect.y < m_rect.y) {
                m_messages.erase(m_messages.begin(), it.base());
                break;
            }
            it->set_rect(text_rect);

            y -= text_rect.h + m_style.text_offset;
        }
    }

    void text_list::render(sdl::renderer &renderer) {
        for (auto &obj : m_messages) {
            obj.render(renderer);
        }
    }

    void text_list::add_message(const std::string &message) {
        m_style.text.wrap_length = m_rect.w;
        m_messages.emplace_back(message, m_style.text);
        set_rect(m_rect);
    }

}