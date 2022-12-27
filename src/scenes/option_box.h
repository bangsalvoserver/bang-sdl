#ifndef __OPTION_BOX_H__
#define __OPTION_BOX_H__

#include "../widgets/checkbox.h"
#include "../widgets/textbox.h"

#include "game/card_enums.h"
#include "utils/parse_string.h"

#include <list>

class lobby_scene;

class option_input_box_base {
public:
    option_input_box_base(lobby_scene *parent) : parent(parent) {}
    virtual ~option_input_box_base() = default;
    virtual sdl::rect get_rect() const = 0;
    virtual void set_pos(sdl::point point) = 0;
    virtual void render(sdl::renderer &renderer) = 0;
    virtual void update_value() = 0;
    virtual void set_locked(bool locked) = 0;

protected:
    lobby_scene *parent;
    void save_value();
};

template<typename T> struct option_input_box;

struct expansion_box : widgets::checkbox {
    expansion_box(const std::string &label, banggame::card_expansion_type flag);
    banggame::card_expansion_type m_flag;
};

template<>
class option_input_box<banggame::card_expansion_type> : public option_input_box_base {
public:
    option_input_box(lobby_scene *parent, const std::string &label, banggame::card_expansion_type &value);

    virtual sdl::rect get_rect() const override;
    virtual void set_pos(sdl::point point) override;
    virtual void render(sdl::renderer &renderer) override;
    virtual void update_value() override;
    virtual void set_locked(bool locked) override;
    
private:
    banggame::card_expansion_type &m_value;

    widgets::stattext m_label;
    std::list<expansion_box> m_checkboxes;
    
    sdl::rect m_rect;
};

template<>
class option_input_box<bool> : public option_input_box_base {
public:
    option_input_box(lobby_scene *parent, const std::string &label, bool &value);

    virtual sdl::rect get_rect() const override;
    virtual void set_pos(sdl::point point) override;
    virtual void render(sdl::renderer &renderer) override;
    virtual void update_value() override;
    virtual void set_locked(bool locked) override;
    
private:
    bool &m_value;

    widgets::checkbox m_checkbox;
};

template<typename T>
concept parsable = requires (std::string_view str, T value) {
    { parse_string<T>(str) } -> std::convertible_to<std::optional<T>>;
    typename fmt::formatter<T>;
};

template<parsable T>
class option_input_box<T> : public option_input_box_base {
public:
    option_input_box(lobby_scene *parent, const std::string &label, T &value)
        : option_input_box_base(parent)
        , m_value(value)
        , m_label(label, widgets::text_style {
            .text_font = &media_pak::font_perdido
        })
    {
        auto do_save_value = [=, this](const std::string &value){
            if (auto parsed = parse_string<T>(value)) {
                m_value = *parsed;
                save_value();
            }
            update_value();
        };
        m_textbox.set_onenter(do_save_value);
        m_textbox.set_onlosefocus(do_save_value);

        update_value();
    }

    virtual sdl::rect get_rect() const override {
        return m_rect;
    }

    virtual void set_pos(sdl::point pt) override {
        m_rect = sdl::rect{pt.x, pt.y, 250, 25};
        m_label.set_rect(sdl::move_rect(m_label.get_rect(), pt));
        auto label_w = m_label.get_rect().w + 10;
        m_textbox.set_rect(sdl::rect{pt.x + label_w, pt.y - 5, m_rect.w - label_w, m_rect.h});
    }

    virtual void render(sdl::renderer &renderer) override {
        m_label.render(renderer);
        m_textbox.render(renderer);
    }

    virtual void update_value() override {
        m_textbox.set_value(fmt::format("{}", m_value));
    }

    virtual void set_locked(bool locked) override {
        m_textbox.set_locked(locked);
    }
    
private:
    T &m_value;

    widgets::stattext m_label;
    widgets::textbox m_textbox;

    sdl::rect m_rect;
};

#endif