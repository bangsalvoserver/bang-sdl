#ifndef __GAME_MESSAGE_BOX_H__
#define __GAME_MESSAGE_BOX_H__

#include <string>

#include "../widgets/button.h"

#include "../intl.h"

namespace banggame {

    class game_ui;

    class game_message_box {
    public:
        game_message_box(
                const std::string &message,
                std::function<void()> &&on_click_yes,
                std::function<void()> &&on_click_no);

        void refresh_layout(const sdl::rect &win_rect);
        void render(sdl::renderer &renderer);

    private:
        widgets::stattext m_message;
        widgets::button m_yes_btn;
        widgets::button m_no_btn;

        sdl::rect m_bg_rect;
    };

}

#endif