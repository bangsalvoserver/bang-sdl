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

}

void make_lobby_scene::render(sdl::renderer &renderer, int w, int h) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    m_username_label.render(renderer);
    
    m_username_box.set_rect(SDL_Rect{100 + label_rect.w + 10, 50, w - 210 - label_rect.w, 25});
    m_username_box.render(renderer);

    label_rect = m_lobbyname_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 100 + (25 - label_rect.h) / 2;
    m_lobbyname_label.set_rect(label_rect);
    m_lobbyname_label.render(renderer);

    m_lobbyname_box.set_rect(SDL_Rect{100 + label_rect.w + 10, 100, w - 210 - label_rect.w, 25});
    m_lobbyname_box.render(renderer);

    m_ok_btn.set_rect(SDL_Rect{100, 150, 100, 25});
    m_ok_btn.render(renderer);

    m_undo_btn.set_rect(SDL_Rect{20, h - 45, 100, 25});
    m_undo_btn.render(renderer);
}

void make_lobby_scene::handle_event(const SDL_Event &event) {
    m_username_box.handle_event(event);
    m_lobbyname_box.handle_event(event);
    m_ok_btn.handle_event(event);
    m_undo_btn.handle_event(event);
}

void make_lobby_scene::do_make_lobby() {
    parent->add_message<client_message_type::lobby_make>(
        m_lobbyname_box.get_value(),
        m_username_box.get_value(),
        enums::flags_all<banggame::card_expansion_type>,
        7);
}