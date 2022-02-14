#include "lobby.h"

#include "../manager.h"

DECLARE_RESOURCE(bkant_bold_ttf)
DECLARE_RESOURCE(perdido_ttf)

using namespace enums::flag_operators;

lobby_player_item::lobby_player_item(int id, const user_info &args)
    : m_name_text(args.name, sdl::text_style{
        .text_font = GET_RESOURCE(bkant_bold_ttf)
    })
    , m_profile_image(&args.profile_image)
    , m_user_id(id) {}

void lobby_player_item::resize(int x, int y) {
    if (m_profile_image && *m_profile_image) {
        m_profile_rect = m_profile_image->get_rect();
        if (m_profile_rect.w > m_profile_rect.h) {
            m_profile_rect.h = m_profile_rect.h * sizes::propic_size / m_profile_rect.h;
            m_profile_rect.w = sizes::propic_size;
        } else {
            m_profile_rect.w = m_profile_rect.w * sizes::propic_size / m_profile_rect.h;
            m_profile_rect.h = sizes::propic_size;
        }

        m_profile_rect.x = x + (sizes::propic_size - m_profile_rect.w) / 2;
        m_profile_rect.y = y + (sizes::propic_size - m_profile_rect.h) / 2;
    }
    m_name_text.set_point(sdl::point{
        x + sizes::propic_size + 10,
        y + (sizes::propic_size - m_name_text.get_rect().h) / 2});
}

void lobby_player_item::render(sdl::renderer &renderer) {
    if (m_profile_image && *m_profile_image) {
        m_profile_image->render(renderer, m_profile_rect);
    }
    m_name_text.render(renderer);
}

expansion_box::expansion_box(const std::string &label, banggame::card_expansion_type flag, banggame::card_expansion_type check)
    : sdl::checkbox(label, sdl::button_style{
        .text = {
            .text_font = GET_RESOURCE(perdido_ttf)
        }
    })
    , m_flag(flag)
{
    using namespace enums::flag_operators;
    set_value(bool(flag & check));
}

lobby_scene::lobby_scene(game_manager *parent)
    : scene_base(parent)
    , m_lobby_name_text(sdl::text_style {
        .text_font = GET_RESOURCE(bkant_bold_ttf)
    })
    , m_leave_btn(_("BUTTON_EXIT"), std::bind(&game_manager::add_message<client_message_type::lobby_leave>, parent))
    , m_start_btn(_("BUTTON_START"), std::bind(&game_manager::add_message<client_message_type::game_start>, parent))
    , m_chat_btn(_("BUTTON_CHAT"), std::bind(&game_manager::enable_chat, parent)) {}

template<banggame::card_expansion_type E> using has_label = std::bool_constant<E != banggame::card_expansion_type::base>;
using card_expansions_with_label = enums::filter_enum_sequence<has_label, enums::make_enum_sequence<banggame::card_expansion_type>>;

void lobby_scene::init(const lobby_entered_args &args) {
    m_lobby_name_text.redraw(args.info.name);
    
    parent->add_message<client_message_type::lobby_players>();
    
    if (parent->get_lobby_owner_id() == parent->get_user_own_id()) {
        [this, expansions = args.info.expansions]
        <banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>){
            (m_checkboxes.emplace_back(_(Es), Es, expansions).set_ontoggle(
                std::bind(&lobby_scene::send_lobby_edited, this)), ...);
        }(card_expansions_with_label());
    } else {
        m_start_btn.disable();

        [this, expansions = args.info.expansions]
        <banggame::card_expansion_type ... Es>(enums::enum_sequence<Es ...>){
            (m_checkboxes.emplace_back(_(Es), Es, expansions).set_locked(true), ...);
        }(card_expansions_with_label());
    }

    resize(parent->width(), parent->height());
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

void lobby_scene::resize(int width, int height) {
    sdl::rect checkbox_rect{130, 60, 0, 25};
    for (auto &box : m_checkboxes) {
        box.set_rect(checkbox_rect);
        checkbox_rect.y += 50;
    }

    m_lobby_name_text.set_point(sdl::point{width / 2 + 5, 20});

    int y = 60;
    for (auto &line : m_player_list) {
        line.resize(width / 2, y);
        y += sizes::propic_size + 10;
    }

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_start_btn.set_rect(sdl::rect{130, 20, 100, 25});
    m_chat_btn.set_rect(sdl::rect{width - 120, height - 40, 100, 25});
}

void lobby_scene::render(sdl::renderer &renderer) {
    m_lobby_name_text.render(renderer);

    for (auto &line : m_player_list) {
        line.render(renderer);
    }

    m_start_btn.render(renderer);
    
    for (auto &box : m_checkboxes) {
        box.render(renderer);
    }

    m_leave_btn.render(renderer);

    m_chat_btn.render(renderer);
}

void lobby_scene::clear_users() {
    m_player_list.clear();
}

void lobby_scene::add_user(int id, const user_info &args) {
    m_player_list.emplace_back(id, args);

    resize(parent->width(), parent->height());
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

    resize(parent->width(), parent->height());
}