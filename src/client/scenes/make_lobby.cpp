#include "make_lobby.h"

#include "../manager.h"

template<banggame::card_expansion_type E> using has_label = std::bool_constant<enums::has_data<E>>;
using card_expansions_with_label = enums::filter_enum_sequence<has_label, enums::make_enum_sequence<banggame::card_expansion_type>>;

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

    [this]<banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>){
        (m_checkboxes.emplace_back(enums::enum_data_v<Es>, Es, this->parent->get_config().expansions), ...);
    }(card_expansions_with_label());
}

void make_lobby_scene::resize(int width, int height) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);

    m_username_box.set_rect(sdl::rect{100 + label_rect.w + 10, 50, width - 210 - label_rect.w, 25});
    
    label_rect = m_lobbyname_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 100 + (25 - label_rect.h) / 2;
    m_lobbyname_label.set_rect(label_rect);

    m_lobbyname_box.set_rect(sdl::rect{100 + label_rect.w + 10, 100, width - 210 - label_rect.w, 25});

    sdl::rect checkbox_rect{100, 150, width - 200, 25};
    for (auto &box : m_checkboxes) {
        box.set_rect(checkbox_rect);
        checkbox_rect.y += 50;
    }
    
    m_ok_btn.set_rect(sdl::rect{checkbox_rect.x, checkbox_rect.y, 100, 25});
    m_undo_btn.set_rect(sdl::rect{20, height - 45, 100, 25});
}

void make_lobby_scene::render(sdl::renderer &renderer) {
    m_username_label.render(renderer);
    m_username_box.render(renderer);
    m_lobbyname_label.render(renderer);
    m_lobbyname_box.render(renderer);

    for (auto &box : m_checkboxes) {
        box.render(renderer);
    }

    m_ok_btn.render(renderer);
    m_undo_btn.render(renderer);
}

void make_lobby_scene::do_make_lobby() {
    using namespace enums::flag_operators;

    parent->get_config().user_name = m_username_box.get_value();
    parent->get_config().lobby_name = m_lobbyname_box.get_value();

    auto expansions = enums::flags_none<banggame::card_expansion_type>;
    for (const auto &box : m_checkboxes) {
        if (box.get_value()) expansions |= box.m_flag;
    }

    parent->get_config().expansions = expansions;
    
    parent->add_message<client_message_type::lobby_make>(
        m_lobbyname_box.get_value(),
        m_username_box.get_value(),
        expansions,
        8);
}