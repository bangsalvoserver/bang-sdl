#ifndef __SCENE_LOBBY_H__
#define __SCENE_LOBBY_H__

#include "scene_base.h"

#include "../widgets/checkbox.h"

#include "game/card_enums.h"

#include <list>

struct expansion_box : widgets::checkbox {
    expansion_box(const std::string &label, banggame::card_expansion_type flag, banggame::card_expansion_type check);
    banggame::card_expansion_type m_flag;
};


class lobby_player_item {
public:
    lobby_player_item(int id, const user_info &args);

    int user_id() const {
        return m_user_id;
    }

    void resize(int x, int y);
    void render(sdl::renderer &renderer);

private:
    widgets::stattext m_name_text;
    const sdl::texture *m_profile_image = nullptr;
    sdl::rect m_profile_rect;

    int m_user_id;
};

class lobby_scene : public scene_base {
public:
    lobby_scene(class game_manager *parent);
    void init(const lobby_entered_args &args);
    void set_lobby_info(const lobby_info &info) override;

    void resize(int width, int height) override;
    void render(sdl::renderer &renderer) override;

    void clear_users() override;
    void add_user(int id, const user_info &args) override;
    void remove_user(int id) override;

    void send_lobby_edited();

private:
    std::vector<lobby_player_item> m_player_list;

    widgets::stattext m_lobby_name_text;
    
    widgets::button m_leave_btn;
    widgets::button m_start_btn;

    widgets::button m_chat_btn;

    std::list<expansion_box> m_checkboxes;

    int m_owner_id = 0;
    int m_user_id = 0;
};

#endif