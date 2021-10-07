#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "utils/sdl.h"
#include "../widgets/button.h"
#include "../widgets/textbox.h"

#include <list>

namespace banggame {

    class message_line {
    public:
        enum message_type {
            chat,
            game,
            error
        };

        message_line(message_type type, const std::string &message);

        void render(sdl::renderer &renderer, const sdl::point &pt);

    private:
        sdl::stattext m_text;
    };

    class game_ui {
    public:
        game_ui(class game_scene *parent);

        void resize(int width, int height);
        void render(sdl::renderer &renderer);

        void add_message(message_line::message_type type, const std::string &message);
        void set_status(const std::string &message) {
            m_status_text.redraw(message);
        }
        void clear_status() {
            m_status_text = sdl::stattext();
        }

        static constexpr int max_messages = 20;

    private:
        class game_scene *parent;

        int m_width;
        int m_height;

        std::list<message_line> m_messages;

        sdl::stattext m_status_text;

        sdl::button m_pass_btn;
        sdl::button m_resolve_btn;
        sdl::button m_leave_btn;
    };

}

#endif