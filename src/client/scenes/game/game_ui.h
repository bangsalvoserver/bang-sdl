#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "../widgets/chat_ui.h"
#include "common/game_action.h"
#include "card.h"

namespace banggame {

    class game_ui {
    public:
        game_ui(class game_scene *parent);

        void resize(int width, int height);
        void render(sdl::renderer &renderer);

        void add_message(const std::string &message);
        void add_game_log(const std::string &message);
        void show_error(const std::string &message);

        void set_status(const std::string &message) {
            m_status_text.redraw(message);
        }
        void clear_status() {
            m_status_text = sdl::stattext();
        }
        void enable_restart(bool value) {
            m_restart_btn.set_enabled(value);
        }

        void add_special(card_view *card);

    private:
        class game_scene *parent;

        chat_ui m_chat;
        sdl::text_list m_game_log;

        sdl::stattext m_status_text;

        sdl::stattext m_error_text;
        int m_error_timeout = 0;

        sdl::button m_pass_btn;
        sdl::button m_resolve_btn;

        std::list<std::pair<sdl::button, card_view*>> m_special_btns;

        sdl::button m_leave_btn;
        sdl::button m_restart_btn;
    };

}

#endif