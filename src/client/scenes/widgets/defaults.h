#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

namespace sdl {

    constexpr uint32_t default_text_rgb = 0x0;
    constexpr uint32_t default_text_bg_rgba = 0xffffff80;
    constexpr int default_text_ptsize = 18;

    constexpr uint32_t default_button_up_rgb = 0xeeeeee;
    constexpr uint32_t default_button_hover_rgb = 0xddddff;
    constexpr uint32_t default_button_down_rgb = 0xccccff;
    constexpr uint32_t default_button_border_rgb = 0x0;
    
    constexpr int chat_text_ptsize = 14;
    constexpr int default_text_list_yoffset = 6;

    constexpr int chat_log_ptsize = 12;

    constexpr uint32_t error_text_rgb = 0xff0000;
    constexpr int error_text_ptsize = 20;

    constexpr uint32_t game_error_text_rgb = 0x800000;

}

#endif