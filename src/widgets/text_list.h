#ifndef __TEXT_LIST_H__
#define __TEXT_LIST_H__

#include <list>
#include "stattext.h"

namespace widgets {

    struct text_list_style {
        text_style text;
        int text_offset = default_text_list_yoffset;
    };

    class text_list {
    private:
        std::list<stattext> m_messages;
        text_list_style m_style;

        sdl::rect m_rect;
        
    public:
        text_list(const text_list_style &style = {}) : m_style(style) {}

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void set_rect(const sdl::rect &rect);

        void render(sdl::renderer &renderer);

        void add_message(const std::string &message);

        void clear();
    };

}

#endif