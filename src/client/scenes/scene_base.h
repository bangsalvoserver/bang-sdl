#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "server/net_enums.h"

#include "utils/sdl.h"

#include "../widgets/button.h"
#include "../widgets/textbox.h"

#include "../user_info.h"

class scene_base {
public:
    scene_base(class client_manager *parent) : parent(parent) {}
    virtual ~scene_base() {}
    
    virtual void refresh_layout() {}

    virtual void render(sdl::renderer &renderer) = 0;
    
    virtual void handle_event(const sdl::event &event) {}

    virtual void set_lobby_info(const lobby_info &args) {}

    virtual void handle_lobby_update(const lobby_data &args) {}

    virtual void add_user(int id, const user_info &args) {}

    virtual void remove_user(int id) {}

    virtual void handle_game_update(const banggame::game_update &update) {}

protected:
    class client_manager *parent;
};

#endif