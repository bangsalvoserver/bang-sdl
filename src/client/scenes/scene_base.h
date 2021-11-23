#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "common/sdl.h"
#include "common/net_enums.h"

#include "widgets/button.h"
#include "widgets/textbox.h"

#include "../user_info.h"

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

    virtual void show_error(const std::string &message) {}

    virtual void set_lobby_list(const std::vector<lobby_data> &args) {}

    virtual void set_lobby_info(const lobby_info &args) {}

    virtual void handle_lobby_update(const lobby_data &args) {}

    virtual void clear_users() {}

    virtual void add_user(int id, const user_info &args) {}

    virtual void remove_user(int id) {}

    virtual void add_chat_message(const std::string &message) {}

    virtual void handle_game_update(const banggame::game_update &update) {}

protected:
    class game_manager *parent;
};

#endif