#ifndef __SCENE_LOBBY_LIST_H__
#define __SCENE_LOBBY_LIST_H__

#include "scene_base.h"

#include <list>

class lobby_line {
public:
    lobby_line(class lobby_list_scene *parent, const lobby_data &args);

    void set_rect(const sdl::rect &rect);
    void render(sdl::renderer &renderer);

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

    void resize(int width, int height) override;
    void render(sdl::renderer &renderer) override;

    void set_lobby_list(const std::vector<lobby_data> &args);

    void refresh();
    void do_join(int lobby_id);
    void do_make_lobby();

private:
    std::list<lobby_line> m_lobby_lines;

    sdl::textbox m_lobby_name_box;
    sdl::button m_make_lobby_btn;

    sdl::button m_disconnect_btn;
    sdl::button m_refresh_btn;
};

#endif