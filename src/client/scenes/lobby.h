#ifndef __SCENE_LOBBY_H__
#define __SCENE_LOBBY_H__

#include "scene_base.h"

class lobby_player_item {
public:
    explicit lobby_player_item(const lobby_player_data &args);

    int user_id() const {
        return m_user_id;
    }

    void render(sdl::renderer &renderer, int x, int y);

private:
    sdl::stattext m_name_text;
    int m_user_id;
};

class lobby_scene : public scene_base {
public:
    lobby_scene(class game_manager *parent);
    void init(const lobby_entered_args &args);

    void render(sdl::renderer &renderer) override;
    void handle_event(const sdl::event &event) override;

    void set_player_list(const std::vector<lobby_player_data> &args);
    void add_user(const lobby_player_data &args);
    void remove_user(const lobby_left_args &args);
    void add_chat_message(const lobby_chat_args &args);

private:
    std::vector<lobby_player_item> m_player_list;
    std::vector<lobby_chat_args> m_messages;

    sdl::stattext m_lobby_name_text;
    
    sdl::button m_leave_btn;
    sdl::button m_start_btn;

    int m_owner_id = 0;
    int m_user_id = 0;
};

#endif