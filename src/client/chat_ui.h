#ifndef __CHAT_UI_H__
#define __CHAT_UI_H__

#include "scenes/widgets/textbox.h"

#include <list>
#include <mutex>

enum class message_type {
    chat,
    error,
    server_log
};

struct chat_message {
    sdl::stattext text;
    int lifetime;
};

struct chat_textbox : sdl::textbox {
    void on_lose_focus() override {
        sdl::textbox::on_lose_focus();

        disable();
    }
};

class chat_ui {
public:
    chat_ui(class game_manager *parent);

    void set_rect(const sdl::rect &rect);
    void render(sdl::renderer &renderer);

    void add_message(message_type, const std::string &message);
    void send_chat_message();

    void enable();
    void disable();
    bool enabled() const;

private:
    class game_manager *parent;

    sdl::text_style get_text_style(message_type type);

    sdl::rect m_rect;

    std::list<chat_message> m_messages;
    mutable std::mutex m_messages_mutex;

    chat_textbox m_chat_box;
};

#endif