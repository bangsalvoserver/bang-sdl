#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

#include "sdl_wrap.h"

#include <chrono>

using duration_type = std::chrono::nanoseconds;

namespace widgets {

    constexpr sdl::color default_text_color = sdl::rgb(0x0);
    constexpr sdl::color default_text_bgcolor = sdl::rgba(0xffffff80);
    constexpr int default_text_ptsize = 18;

    constexpr sdl::color default_button_up_color = sdl::rgb(0xeeeeee);
    constexpr sdl::color default_button_hover_color = sdl::rgb(0xddddff);
    constexpr sdl::color default_button_down_color = sdl::rgb(0xccccff);
    constexpr sdl::color default_button_border_color = sdl::rgb(0x0);

    constexpr sdl::color default_textbox_background_color = sdl::rgb(0xffffff);
    constexpr sdl::color default_textbox_border_color = sdl::rgb(0x0);
    constexpr sdl::color default_textbox_selection_color = sdl::rgb(0x00c0ff);
    
    constexpr std::chrono::milliseconds chat_message_lifetime{15000};

    constexpr int chat_text_ptsize = 12;
    constexpr int default_text_list_yoffset = 6;

    constexpr int chat_log_ptsize = 12;

    constexpr sdl::color error_text_color = sdl::rgb(0xff0000);
    constexpr int error_text_ptsize = 14;

    constexpr sdl::color server_log_color = sdl::rgb(0x0000ff);

    constexpr sdl::color propic_border_color = sdl::rgba(0xa79c78ff);

}

#endif