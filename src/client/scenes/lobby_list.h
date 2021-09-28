#ifndef __SCENE_LOBBY_LIST_H__
#define __SCENE_LOBBY_LIST_H__

#include "scene_base.h"

class lobby_line {
public:
    lobby_line(class lobby_list_scene *parent, const lobby_data &args);

    void render(sdl::renderer &renderer, const SDL_Rect &rect);
    void handle_event(const SDL_Event &event);

private:
    class lobby_list_scene *parent;

    sdl::stattext m_name_text;
    sdl::stattext m_players_text;
    sdl::stattext m_state_text;

    sdl::button m_join_btn;
};

class lobby_list_scene : public scene_base {
public:
    lobby_list_scene(class game_manager *parent);

    void render(sdl::renderer &renderer) override;
    void handle_event(const SDL_Event &event) override;

    void set_lobby_list(const std::vector<lobby_data> &args);

    void refresh();
    void do_join(int lobby_id);

private:
    std::vector<lobby_line> m_lobby_lines;

    sdl::button m_disconnect_btn;
    sdl::button m_refresh_btn;
    sdl::button m_make_lobby_btn;

    sdl::stattext m_username_label;
    sdl::textbox m_username_box;
};

#endif