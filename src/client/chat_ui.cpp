#include "chat_ui.h"

#include "manager.h"

chat_ui::chat_ui(game_manager *parent)
    : parent(parent)
{
    m_chat_box.set_onenter(std::bind(&chat_ui::send_chat_message, this));

    m_chat_box.disable();
}

void chat_ui::set_rect(const sdl::rect &rect) {
    m_rect = rect;

    int y = rect.y + rect.h - 35;
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
        sdl::rect text_rect = it->text.get_rect();
        text_rect.x = m_rect.x;
        text_rect.y = y - text_rect.h;
        if (text_rect.y < m_rect.y) {
            m_messages.erase(m_messages.begin(), it.base());
            break;
        }
        it->text.set_rect(text_rect);

        y -= text_rect.h + sdl::default_text_list_yoffset;
    }

    m_chat_box.set_rect(sdl::rect{rect.x, rect.y + rect.h - 25, rect.w, 25});
}

void chat_ui::render(sdl::renderer &renderer) {
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
        if (--it->lifetime <= 0) {
            m_messages.erase(m_messages.begin(), it.base());
            break;
        }
        it->text.render(renderer);
    }
    if (m_chat_box.enabled()) {
        m_chat_box.render(renderer);
    }
}

sdl::text_style chat_ui::get_text_style(message_type type) {
    switch (type) {
    case message_type::error:
        return {
            .text_color = sdl::rgb(sdl::error_text_rgb),
            .text_ptsize = sdl::error_text_ptsize,
            .wrap_length = m_rect.w
        };
    case message_type::server_log:
        return {
            .text_color = sdl::rgb(sdl::server_log_rgb),
            .text_ptsize = sdl::chat_text_ptsize,
            .wrap_length = m_rect.w
        };
    case message_type::chat:
    default:
        return {
            .text_ptsize = sdl::chat_text_ptsize,
            .wrap_length = m_rect.w
        };
    }
}

void chat_ui::add_message(message_type type, const std::string &message) {
    m_messages.emplace_back(sdl::stattext{message, get_text_style(type)}, sdl::chat_message_lifetime);
    set_rect(m_rect);
}

void chat_ui::send_chat_message() {
    if (!m_chat_box.get_value().empty()) {
        parent->add_message<client_message_type::lobby_chat>(m_chat_box.get_value());
        m_chat_box.set_value("");
    }
}

void chat_ui::enable() {
    m_chat_box.enable();
    sdl::event_handler::set_focus(&m_chat_box);
}

void chat_ui::disable() {
    m_chat_box.disable();
}

bool chat_ui::enabled() const {
    return m_chat_box.enabled();
}