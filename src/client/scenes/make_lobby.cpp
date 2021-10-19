#include "make_lobby.h"

#include "../manager.h"

make_lobby_scene::make_lobby_scene(game_manager *parent)
    : scene_base(parent)
    , m_undo_btn("Annulla", [parent]{
        parent->switch_scene<scene_type::lobby_list>();
    })
    , m_ok_btn("OK", [this]{
        do_make_lobby();
    })
    , m_username_label("Nome utente:")
    , m_lobbyname_label("Nome lobby:")
{
    m_username_box.set_value(parent->get_config().user_name);
    m_lobbyname_box.set_value(parent->get_config().lobby_name);
}

void make_lobby_scene::render(sdl::renderer &renderer) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    m_username_label.render(renderer);
    
    m_username_box.set_rect(sdl::rect{100 + label_rect.w + 10, 50, parent->width() - 210 - label_rect.w, 25});
    m_username_box.render(renderer);

    label_rect = m_lobbyname_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 100 + (25 - label_rect.h) / 2;
    m_lobbyname_label.set_rect(label_rect);
    m_lobbyname_label.render(renderer);

    m_lobbyname_box.set_rect(sdl::rect{100 + label_rect.w + 10, 100, parent->width() - 210 - label_rect.w, 25});
    m_lobbyname_box.render(renderer);

    m_ok_btn.set_rect(sdl::rect{100, 150, 100, 25});
    m_ok_btn.render(renderer);

    m_undo_btn.set_rect(sdl::rect{20, parent->height() - 45, 100, 25});
    m_undo_btn.render(renderer);
}

void make_lobby_scene::do_make_lobby() {
    parent->get_config().user_name = m_username_box.get_value();
    parent->get_config().lobby_name = m_lobbyname_box.get_value();
    
    parent->add_message<client_message_type::lobby_make>(
        m_lobbyname_box.get_value(),
        m_username_box.get_value(),
        enums::flags_all<banggame::card_expansion_type>,
        8);
}