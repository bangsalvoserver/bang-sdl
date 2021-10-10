#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "../widgets/chat_ui.h"

namespace banggame {

    class game_ui {
    public:
        game_ui(class game_scene *parent);

        void resize(int width, int height);
        void render(sdl::renderer &renderer);

        void add_message(const std::string &message);
        void show_error(const std::string &message);

        void set_status(const std::string &message) {
            m_status_text.redraw(message);
        }
        void clear_status() {
            m_status_text = sdl::stattext();
        }

    private:
        class game_scene *parent;

        chat_ui m_chat;

        sdl::stattext m_status_text;

        sdl::stattext m_error_text;
        int m_error_timeout;

        sdl::button m_pass_btn;
        sdl::button m_resolve_btn;
        sdl::button m_leave_btn;
    };

}

#endif