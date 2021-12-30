#include "chat_ui.h"

#include "../../manager.h"

chat_ui::chat_ui(game_manager *parent)
    : parent(parent)
    , m_send_btn(_("BUTTON_SEND"), [this] {
        send_chat_message();
    })
    , m_messages(sdl::text_list_style{
        .text = {
            .text_ptsize = sdl::chat_text_ptsize
        }
    })
{
    m_chat_box.set_onenter([this] {
        send_chat_message();
    });
}

void chat_ui::resize(int width, int height) {
    m_chat_box.set_rect(sdl::rect{20, height - 50, 200, 25});
    m_send_btn.set_rect(sdl::rect{230, height - 50, 100, 25});
    m_messages.set_rect(sdl::rect{20, 300, 300, height - 370});
}

void chat_ui::render(sdl::renderer &renderer) {
    m_messages.render(renderer);
    m_chat_box.render(renderer);
    m_send_btn.render(renderer);
}

void chat_ui::add_message(const std::string &message) {
    m_messages.add_message(message);
}

void chat_ui::send_chat_message() {
    if (!m_chat_box.get_value().empty()) {
        parent->add_message<client_message_type::lobby_chat>(m_chat_box.get_value());
        m_chat_box.set_value("");
    }
}