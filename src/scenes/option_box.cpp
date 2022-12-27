#include "option_box.h"

#include "../manager.h"
#include "../media_pak.h"

#include "lobby.h"

using namespace banggame;

void option_input_box_base::save_value() {
    parent->send_lobby_edited();
}

expansion_box::expansion_box(const std::string &label, banggame::card_expansion_type flag)
    : widgets::checkbox(label, widgets::button_style{
        .text = {
            .text_font = &media_pak::font_perdido,
            .bg_color = sdl::rgba(0)
        }
    })
    , m_flag(flag)
{}

option_input_box<banggame::card_expansion_type>::option_input_box(lobby_scene *parent, const std::string &label, banggame::card_expansion_type &value)
    : option_input_box_base(parent)
    , m_value(value)
    , m_label(label, widgets::text_style {
        .text_font = &media_pak::font_perdido,
        .bg_color = sdl::rgba(0)
    })
{
    for (auto E : enums::enum_values_v<banggame::card_expansion_type>) {
        if (!parent->manager()->get_config().allow_unofficial_expansions && bool(banggame::unofficial_expansions & E)) continue;

        m_checkboxes.emplace_back(_(E), E).set_ontoggle([=, this](bool value){
            if (value) {
                m_value |= E;
            } else {
                m_value &= ~E;
            }
            save_value();
        });
    }

    update_value();
}

sdl::rect option_input_box<banggame::card_expansion_type>::get_rect() const {
    return m_rect;
}

void option_input_box<banggame::card_expansion_type>::set_pos(sdl::point pt) {
    m_rect = sdl::rect{pt.x, pt.y, 250, 25};

    m_label.set_rect(sdl::move_rect(m_label.get_rect(), pt));

    sdl::rect checkbox_rect{pt.x, pt.y + 30, 0, 25};
    for (auto &checkbox : m_checkboxes) {
        checkbox.set_rect(checkbox_rect);
        m_rect.h += 30;
        checkbox_rect.y += 30;
    }
}

void option_input_box<banggame::card_expansion_type>::render(sdl::renderer &renderer) {
    renderer.set_draw_color(sdl::rgba(0xffffff40));
    renderer.fill_rect(sdl::rect{m_rect.x - 5, m_rect.y - 5, m_rect.w + 10, m_rect.h + 10});

    m_label.render(renderer);
    for (auto &checkbox : m_checkboxes) {
        checkbox.render(renderer);
    }
}

void option_input_box<banggame::card_expansion_type>::update_value() {
    for (auto &checkbox : m_checkboxes) {
        checkbox.set_value(bool(m_value & checkbox.m_flag));
    }
}

void option_input_box<banggame::card_expansion_type>::set_locked(bool locked) {
    for (auto &checkbox : m_checkboxes) {
        checkbox.set_locked(locked);
    }
}

option_input_box<bool>::option_input_box(lobby_scene *parent, const std::string &label, bool &value)
    : option_input_box_base(parent)
    , m_value(value)
    , m_checkbox(_(label), widgets::button_style{
        .text = {
            .text_font = &media_pak::font_perdido
        }
    })
{
    m_checkbox.set_ontoggle([=, this](bool value) {
        m_value = value;
        save_value();
    });

    update_value();
}

sdl::rect option_input_box<bool>::get_rect() const {
    return m_checkbox.get_rect();
}

void option_input_box<bool>::set_pos(sdl::point pt) {
    m_checkbox.set_rect(sdl::rect(pt.x, pt.y, 0, 25));
}

void option_input_box<bool>::render(sdl::renderer &renderer) {
    m_checkbox.render(renderer);
}

void option_input_box<bool>::update_value() {
    m_checkbox.set_value(m_value);
}

void option_input_box<bool>::set_locked(bool locked) {
    m_checkbox.set_locked(locked);
}