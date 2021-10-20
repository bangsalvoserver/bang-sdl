#include "chat_ui.h"

#include "../../manager.h"

chat_ui::chat_ui(game_manager *parent)
    : parent(parent)
    , m_send_btn("Invia", [this] {
        send_chat_message();
    })
{
    m_chat_box.set_onenter([this] {
        send_chat_message();
    });
}

void chat_ui::resize(int width, int height) {
    m_chat_box.set_rect(sdl::rect{20, height - 50, 200, 25});
    m_send_btn.set_rect(sdl::rect{230, height - 50, 100, 25});
}

void chat_ui::render(sdl::renderer &renderer) {
    sdl::point pt{20, parent->height() - 80};
    for (auto &line : m_messages | std::views::reverse) {
        line.set_point(pt);
        line.render(renderer);
        pt.y -= 20;
    }

    m_chat_box.render(renderer);
    m_send_btn.render(renderer);
}

void chat_ui::add_message(const std::string &message) {
    m_messages.emplace_back(message);
    if (m_messages.size() > max_messages) {
        m_messages.pop_front();
    }
}

void chat_ui::send_chat_message() {
    if (!m_chat_box.get_value().empty()) {
        parent->add_message<client_message_type::lobby_chat>(m_chat_box.get_value());
        m_chat_box.set_value("");
    }
}