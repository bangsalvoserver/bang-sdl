#ifndef __SCENE_LOBBY_LIST_H__
#define __SCENE_LOBBY_LIST_H__

#include "scene_base.h"

#include <list>

class lobby_line {
public:
    lobby_line(class lobby_list_scene *parent, const lobby_data &args);

    void set_rect(const sdl::rect &rect);
    void render(sdl::renderer &renderer);
    
    int id() const {
        return m_id;
    }

private:
    class lobby_list_scene *parent;

    int m_id;

    widgets::stattext m_name_text;
    widgets::stattext m_players_text;
    widgets::stattext m_state_text;

    widgets::button m_join_btn;
};

class lobby_list_scene : public scene_base {
public:
    lobby_list_scene(class client_manager *parent);

    void resize(int width, int height) override;
    void render(sdl::renderer &renderer) override;

    void set_lobby_list(const std::vector<lobby_data> &args) override;
    void handle_lobby_update(const lobby_data &args);

    void do_join(int lobby_id);
    void do_make_lobby();

private:
    std::list<lobby_line> m_lobby_lines;

    widgets::textbox m_lobby_name_box;
    widgets::button m_make_lobby_btn;

    widgets::button m_disconnect_btn;
};

#endif