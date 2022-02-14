#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "common/game_action.h"
#include "card.h"

#include "../widgets/text_list.h"
#include "../widgets/button.h"

namespace banggame {

    class game_ui {
    public:
        game_ui(class game_scene *parent);

        void resize(int width, int height);
        void render(sdl::renderer &renderer);

        void add_game_log(const std::string &message);

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

        sdl::text_list m_game_log;

        sdl::stattext m_status_text;

        sdl::button m_confirm_btn;

        std::list<std::pair<sdl::button, card_view*>> m_special_btns;

        sdl::button m_leave_btn;
        sdl::button m_restart_btn;

        sdl::button m_chat_btn;
    };

}

#endif