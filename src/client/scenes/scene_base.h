#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "utils/sdl.h"
#include "common/net_enums.h"

#include "widgets/button.h"
#include "widgets/textbox.h"

class scene_base {
public:
    scene_base(class game_manager *parent) : parent(parent) {}
    virtual ~scene_base() {}

    virtual SDL_Color bg_color() {
        return {0xff, 0xff, 0xff, 0xff};
    }
    
    virtual void resize(int width, int height) {
        m_width = width;
        m_height = height;
    }
    virtual void render(sdl::renderer &renderer) = 0;
    virtual void handle_event(const SDL_Event &event) = 0;

protected:
    class game_manager *parent;

    int m_width;
    int m_height;
};

#endif