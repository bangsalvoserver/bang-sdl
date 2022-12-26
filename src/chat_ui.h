#ifndef __CHAT_UI_H__
#define __CHAT_UI_H__

#include "widgets/textbox.h"
#include "utils/tsqueue.h"

#include <list>
#include <mutex>

enum class message_type {
    chat,
    error,
    server_log
};

struct chat_message {
    widgets::stattext text;
    duration_type lifetime;
};

struct chat_textbox : widgets::textbox {
    void on_lose_focus() override {
        widgets::textbox::on_lose_focus();

        disable();
    }
};

class client_manager;

class chat_ui {
public:
    chat_ui(client_manager *parent);

    void set_rect(const sdl::rect &rect);
    void tick(duration_type time_elapsed);
    void render(sdl::renderer &renderer);

    void add_message(message_type, const std::string &message);
    void send_chat_message(const std::string &value);

    void enable();
    void disable();
    bool enabled() const;

private:
    client_manager *parent;

    widgets::text_style get_text_style(message_type type);

    sdl::rect m_rect;

    std::list<chat_message> m_messages;

    static constexpr size_t max_messages = 100;
    util::tsqueue<std::pair<message_type, std::string>, max_messages> m_pending_messages;

    chat_textbox m_chat_box;
};

#endif