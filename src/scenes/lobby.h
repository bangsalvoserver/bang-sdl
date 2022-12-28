#ifndef __SCENE_LOBBY_H__
#define __SCENE_LOBBY_H__

#include "scene_base.h"

#include "../widgets/profile_pic.h"
#include "option_box.h"

#include <list>

struct user_info;

class lobby_scene : public scene_base,
public message_handler<banggame::server_message_type::lobby_edited>,
public message_handler<banggame::server_message_type::lobby_owner>,
public message_handler<banggame::server_message_type::lobby_add_user>,
public message_handler<banggame::server_message_type::lobby_remove_user> {
public:
    lobby_scene(client_manager *parent, const banggame::lobby_info &args);

    void refresh_layout() override;
    void render(sdl::renderer &renderer) override;
    void handle_event(const sdl::event &event) override;

    void handle_message(SRV_TAG(lobby_edited), const banggame::lobby_info &info) override;
    void handle_message(SRV_TAG(lobby_owner), const banggame::user_id_args &args) override;
    void handle_message(SRV_TAG(lobby_add_user), const banggame::lobby_add_user_args &args) override;
    void handle_message(SRV_TAG(lobby_remove_user), const banggame::user_id_args &args) override;

    void send_lobby_edited();

private:
    class lobby_player_item {
    public:
        lobby_player_item(lobby_scene *parent, int id, const user_info &args);

        int user_id() const {
            return m_user_id;
        }

        void set_pos(int x, int y);
        void render(sdl::renderer &renderer);

    private:
        lobby_scene *parent;

        int m_user_id;

        widgets::stattext m_name_text;
        widgets::profile_pic m_propic;
    };

    std::vector<lobby_player_item> m_player_list;

    widgets::stattext m_lobby_name_text;
    
    widgets::button m_leave_btn;
    widgets::button m_start_btn;

    widgets::button m_chat_btn;

    banggame::game_options m_lobby_options;

    std::vector<std::unique_ptr<option_input_box_base>> m_option_boxes;

    int m_owner_id = 0;
    int m_user_id = 0;
};

#endif