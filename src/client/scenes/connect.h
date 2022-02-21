#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "scene_base.h"

#include "../widgets/profile_pic.h"

#include <list>

struct recent_server_line {
    class connect_scene *parent;
    
    widgets::stattext m_address_text;
    widgets::button m_connect_btn;
    widgets::button m_delete_btn;

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

    void do_connect(const std::string &address);

    void do_delete_address(recent_server_line *addr);

    void do_browse_propic();

    void do_create_server();

private:
    widgets::stattext m_username_label;
    widgets::textbox m_username_box;
    widgets::profile_pic m_propic;

    widgets::stattext m_address_label;
    widgets::textbox m_address_box;
    widgets::button m_connect_btn;
    widgets::button m_create_server_btn;

    std::list<recent_server_line> m_recents;
};

#endif