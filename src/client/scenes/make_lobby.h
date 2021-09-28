#ifndef __SCENE_MAKE_LOBBY_H__
#define __SCENE_MAKE_LOBBY_H__

#include "scene_base.h"

class make_lobby_scene : public scene_base {
public:
    make_lobby_scene(class game_manager *parent);

    void render(sdl::renderer &renderer) override;
    void handle_event(const SDL_Event &event) override;

    void do_make_lobby();

private:
    sdl::button m_undo_btn;
    sdl::button m_ok_btn;

    sdl::stattext m_username_label;
    sdl::stattext m_lobbyname_label;

    sdl::textbox m_username_box;
    sdl::textbox m_lobbyname_box;
};

#endif