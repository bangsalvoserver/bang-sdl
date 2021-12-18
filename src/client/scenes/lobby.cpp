#include "lobby.h"

#include "../manager.h"

using namespace enums::flag_operators;

lobby_player_item::lobby_player_item(int id, const user_info &args)
    : m_name_text(args.name)
    , m_profile_image(&args.profile_image)
    , m_user_id(id) {}

void lobby_player_item::render(sdl::renderer &renderer, int x, int y) {
    if (m_profile_image && *m_profile_image) {
        sdl::rect rect = m_profile_image->get_rect();
        if (rect.w > rect.h) {
            rect.h = rect.h * sizes::propic_size / rect.h;
            rect.w = sizes::propic_size;
        } else {
            rect.w = rect.w * sizes::propic_size / rect.h;
            rect.h = sizes::propic_size;
        }

        rect.x = x + (sizes::propic_size - rect.w) / 2;
        rect.y = y + (sizes::propic_size - rect.h) / 2;
        m_profile_image->render(renderer, rect);
    }
    m_name_text.set_point(sdl::point{
        x + sizes::propic_size + 10,
        y + (sizes::propic_size - m_name_text.get_rect().h) / 2});
    m_name_text.render(renderer);
}

template<banggame::card_expansion_type E> using has_label = std::bool_constant<enums::has_data<E>>;
using card_expansions_with_label = enums::filter_enum_sequence<has_label, enums::make_enum_sequence<banggame::card_expansion_type>>;

lobby_scene::lobby_scene(game_manager *parent)
    : scene_base(parent)
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{
        parent->add_message<client_message_type::lobby_leave>();
    })
    , m_start_btn(_("BUTTON_START"), [parent]{
        parent->add_message<client_message_type::game_start>();
    })
    , m_chat(parent) {}

void lobby_scene::init(const lobby_entered_args &args) {
    m_lobby_name_text.redraw(args.info.name);
    
    parent->add_message<client_message_type::lobby_players>();
    
    if (parent->get_lobby_owner_id() == parent->get_user_own_id()) {
        [this, expansions = args.info.expansions]
        <banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>){
            (m_checkboxes.emplace_back(enums::enum_data_v<Es>, Es, expansions).set_ontoggle([this] {
                send_lobby_edited();
            }), ...);
        }(card_expansions_with_label());
    } else {
        m_start_btn.disable();

        [this, expansions = args.info.expansions]
        <banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>){
            (m_checkboxes.emplace_back(enums::enum_data_v<Es>, Es, expansions).set_locked(true), ...);
        }(card_expansions_with_label());
    }
}

void lobby_scene::set_lobby_info(const lobby_info &info) {
    m_lobby_name_text.redraw(info.name);
    
    [expansions = info.expansions, it = m_checkboxes.begin()]
    <banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>) mutable {
        ((it++)->set_value(bool(Es & expansions)), ...);
    }(card_expansions_with_label());
}

void lobby_scene::send_lobby_edited() {
    auto expansions = enums::flags_none<banggame::card_expansion_type>;
    for (const auto &box : m_checkboxes) {
        if (box.get_value()) {
            expansions |= box.m_flag;
        }
    }
    parent->get_config().expansions = expansions;
    parent->add_message<client_message_type::lobby_edit>(m_lobby_name_text.get_value(), expansions);
}

void lobby_scene::render(sdl::renderer &renderer) {
    if (m_lobby_name_text) {
        sdl::rect rect = m_lobby_name_text.get_rect();
        rect.x = (parent->width() - rect.w) / 2;
        rect.y = 60;
        m_lobby_name_text.set_rect(rect);
        m_lobby_name_text.render(renderer);
    }

    int y = 100;
    for (auto &line : m_player_list) {
        line.render(renderer, 100, y);
        y += sizes::propic_size + 10;
    }

    m_start_btn.set_rect(sdl::rect{100, y, 100, 25});
    m_start_btn.render(renderer);
    
    sdl::rect checkbox_rect{parent->width() / 2, 100, 0, 25};
    for (auto &box : m_checkboxes) {
        box.set_rect(checkbox_rect);
        box.render(renderer);
        checkbox_rect.y += 50;
    }

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_leave_btn.render(renderer);

    m_chat.resize(parent->width(), parent->height());
    m_chat.render(renderer);
}

void lobby_scene::clear_users() {
    m_player_list.clear();
}

void lobby_scene::add_user(int id, const user_info &args) {
    m_player_list.emplace_back(id, args);
}

void lobby_scene::remove_user(int id) {
    if (id == m_user_id) {
        parent->switch_scene<scene_type::lobby_list>();
    } else {
        auto it = std::ranges::find(m_player_list, id, &lobby_player_item::user_id);
        if (it != m_player_list.end()) {
            m_player_list.erase(it);
        }
    }
}

void lobby_scene::add_chat_message(const std::string &message) {
    m_chat.add_message(message);
}