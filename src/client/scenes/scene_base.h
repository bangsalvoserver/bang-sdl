#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "utils/sdl.h"
#include "common/net_enums.h"

#include "widgets/button.h"
#include "widgets/textbox.h"

class scene_base {
public:
    scene_base(class game_manager *parent) : parent(parent) {}

    virtual SDL_Color bg_color() {
        return {0xff, 0xff, 0xff, 0xff};
    }
    
    virtual void render(sdl::renderer &renderer, int w, int h) = 0;
    virtual void handle_event(const SDL_Event &event) = 0;

protected:
    class game_manager *parent;
};

#endif