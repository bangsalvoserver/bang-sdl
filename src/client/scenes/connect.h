#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "scene_base.h"

class connect_scene : public scene_base {
public:
    connect_scene(class game_manager *parent);
    
    void render(sdl::renderer &renderer) override;
    void handle_event(const SDL_Event &event) override;

    void show_error(const std::string &message);

private:
    sdl::textbox m_address_box;
    sdl::button m_connect_btn;
    sdl::stattext m_error_text;
};

#endif