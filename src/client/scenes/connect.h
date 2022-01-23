#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "scene_base.h"

#include <list>

struct recent_server_line {
    class connect_scene *parent;
    
    sdl::stattext m_address_text;
    sdl::button m_connect_btn;
    sdl::button m_delete_btn;

    recent_server_line(class connect_scene *parent, const std::string &address);

    void set_rect(const sdl::rect &rect);
    void render(sdl::renderer &renderer);
    void handle_event(const sdl::event &event);
};

class connect_scene : public scene_base {
public:
    connect_scene(class game_manager *parent);
    
    void resize(int width, int height) override;
    
    void render(sdl::renderer &renderer) override;

    void show_error(const std::string &message) override;

    void do_connect(const std::string &address);

    void do_delete_address(recent_server_line *addr);

    void do_browse();

    void do_create_server();

private:
    sdl::stattext m_username_label;
    sdl::textbox m_username_box;

    sdl::stattext m_propic_label;
    sdl::textbox m_propic_box;
    sdl::button m_propic_browse_btn;

    sdl::stattext m_address_label;
    sdl::textbox m_address_box;
    sdl::button m_connect_btn;
    sdl::button m_create_server_btn;

    sdl::stattext m_error_text;

    std::list<recent_server_line> m_recents;
};

#endif