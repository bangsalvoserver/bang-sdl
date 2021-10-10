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

    virtual sdl::color bg_color() {
        return {0xff, 0xff, 0xff, 0xff};
    }
    
    virtual void resize(int width, int height) {}

    virtual void render(sdl::renderer &renderer) = 0;
    
    virtual void handle_event(const sdl::event &event) {}

protected:
    class game_manager *parent;
};

#endif