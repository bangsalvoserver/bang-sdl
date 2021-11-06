#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "../widgets/chat_ui.h"
#include "common/game_action.h"

namespace banggame {

    class game_ui {
    public:
        game_ui(class game_scene *parent);

        void resize(int width, int height);
        void render(sdl::renderer &renderer);

        void set_button_flags(play_card_flags flags);

        void add_message(const std::string &message);
        void show_error(const std::string &message);

        void set_status(const std::string &message) {
            m_status_text.redraw(message);
        }
        void clear_status() {
            m_status_text = sdl::stattext();
        }
        void disable_restart() {
            m_restart_btn.disable();
        }
        void disable_goldrush() {
            m_sell_beer_btn.disable();
            m_discard_black_btn.disable();
        }

    private:
        class game_scene *parent;

        chat_ui m_chat;

        sdl::stattext m_status_text;

        sdl::stattext m_error_text;
        int m_error_timeout;

        sdl::button m_pass_btn;
        sdl::button m_resolve_btn;
        sdl::button m_sell_beer_btn;
        sdl::button m_discard_black_btn;

        sdl::button m_leave_btn;
        sdl::button m_restart_btn;
    };

}

#endif