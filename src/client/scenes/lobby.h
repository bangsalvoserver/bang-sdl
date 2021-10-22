#ifndef __SCENE_LOBBY_H__
#define __SCENE_LOBBY_H__

#include "scene_base.h"

#include "widgets/chat_ui.h"
#include "widgets/checkbox.h"

#include "common/card_enums.h"

struct expansion_box : sdl::checkbox {
    expansion_box(const std::string &label, banggame::card_expansion_type flag, banggame::card_expansion_type check)
        : sdl::checkbox(label)
        , m_flag(flag)
    {
        using namespace enums::flag_operators;
        set_value(bool(flag & check));
    }

    banggame::card_expansion_type m_flag;
};


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
    void set_lobby_info(const lobby_info &info);

    void render(sdl::renderer &renderer) override;

    void set_player_list(const std::vector<lobby_player_data> &args);
    void add_user(const lobby_player_data &args);
    void remove_user(const lobby_left_args &args);
    void add_chat_message(const std::string &message);

    void send_lobby_edited();

private:
    std::vector<lobby_player_item> m_player_list;

    sdl::stattext m_lobby_name_text;
    
    sdl::button m_leave_btn;
    sdl::button m_start_btn;

    std::list<expansion_box> m_checkboxes;

    chat_ui m_chat;

    int m_owner_id = 0;
    int m_user_id = 0;
};

#endif