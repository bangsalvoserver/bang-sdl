#ifndef __GAME_MESSAGE_BOX_H__
#define __GAME_MESSAGE_BOX_H__

#include <string>
#include <list>

#include "../widgets/button.h"

#include "../intl.h"

namespace banggame {

    class game_ui;

    using button_init_list = std::initializer_list<std::pair<std::string, std::function<void()>>>;

    class game_message_box {
    public:
        game_message_box(game_ui *parent, const std::string &message, button_init_list &&buttons);

        void refresh_layout(const sdl::rect &win_rect);
        void render(sdl::renderer &renderer);

    private:
        widgets::stattext m_message;
        std::list<widgets::button> m_buttons;

        sdl::rect m_bg_rect;
    };

}

#endif