#ifndef __SCENE_MAKE_LOBBY_H__
#define __SCENE_MAKE_LOBBY_H__

#include "scene_base.h"
#include "widgets/checkbox.h"

#include "common/card_enums.h"

#include <list>

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

class make_lobby_scene : public scene_base {
public:
    make_lobby_scene(class game_manager *parent);

    void resize(int width, int height) override;

    void render(sdl::renderer &renderer) override;

    void do_make_lobby();

private:
    sdl::button m_undo_btn;
    sdl::button m_ok_btn;

    sdl::stattext m_username_label;
    sdl::stattext m_lobbyname_label;

    sdl::textbox m_username_box;
    sdl::textbox m_lobbyname_box;

    std::list<expansion_box> m_checkboxes;
};

#endif