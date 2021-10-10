#ifndef __CHAT_UI_H__
#define __CHAT_UI_H__

#include <list>

#include "utils/sdl.h"
#include "textbox.h"
#include "button.h"

class chat_ui {
public:
    chat_ui(class game_manager *parent);

    void resize(int width, int height);
    void render(sdl::renderer &renderer);

    void add_message(const std::string &message);
    void send_chat_message();

    static constexpr int max_messages = 20;

private:
    class game_manager *parent;

    std::list<sdl::stattext> m_messages;
    sdl::textbox m_chat_box;
    sdl::button m_send_btn;
};

#endif