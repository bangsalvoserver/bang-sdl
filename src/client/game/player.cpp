#include "player.h"

DECLARE_RESOURCE(bkant_bold_ttf)

namespace banggame {
    player_view::player_view(int id)
        : id(id)
        , m_username_text(widgets::text_style{
            .text_font = GET_RESOURCE(bkant_bold_ttf)
        }) {}

    void player_view::set_position(sdl::point pos, bool flipped) {
        m_bounding_rect.w = table.width() + options::card_width * 3 + 40;
        m_bounding_rect.h = options::player_view_height;
        m_bounding_rect.x = pos.x - m_bounding_rect.w / 2;
        m_bounding_rect.y = pos.y - m_bounding_rect.h / 2;
        table.set_pos(sdl::point{
            m_bounding_rect.x + (table.width() + options::card_width) / 2 + 10,
            m_bounding_rect.y + m_bounding_rect.h / 2});
        hand.set_pos(table.get_pos());
        m_characters.set_pos(sdl::point{
            m_bounding_rect.x + m_bounding_rect.w - options::card_width - options::card_width / 2 - 20,
            m_bounding_rect.y + m_bounding_rect.h - options::card_width - 10});
        m_role.set_pos(sdl::point(
            m_characters.get_pos().x + options::card_width + 10,
            m_characters.get_pos().y + 16));
        set_hp_marker_position(hp);
        if (flipped) {
            hand.set_pos(sdl::point{hand.get_pos().x, hand.get_pos().y + options::card_yoffset});
            table.set_pos(sdl::point{table.get_pos().x, table.get_pos().y - options::card_yoffset});
        } else {
            table.set_pos(sdl::point{table.get_pos().x, table.get_pos().y + options::card_yoffset});
            hand.set_pos(sdl::point{hand.get_pos().x, hand.get_pos().y - options::card_yoffset});
        }

        set_username(m_username_text.get_value());
        if (m_profile_image) {
            set_profile_image(m_profile_image);
        }
    }

    void player_view::set_username(const std::string &value) {
        m_username_text.redraw(value);
        sdl::rect username_rect = m_username_text.get_rect();
        username_rect.x = m_role.get_pos().x - (username_rect.w) / 2;
        username_rect.y = m_bounding_rect.y + 43;
        m_username_text.set_rect(username_rect);
    }

    void player_view::set_profile_image(sdl::texture *image) {
        m_profile_image = image;
        if (!image ||!*image) return;

        m_profile_rect = m_profile_image->get_rect();
        if (m_profile_rect.w > m_profile_rect.h) {
            m_profile_rect.h = m_profile_rect.h * widgets::propic_size / m_profile_rect.w;
            m_profile_rect.w = widgets::propic_size;
        } else {
            m_profile_rect.w = m_profile_rect.w * widgets::propic_size / m_profile_rect.h;
            m_profile_rect.h = widgets::propic_size;
        }

        m_profile_rect.x = m_role.get_pos().x - m_profile_rect.w / 2;
        m_profile_rect.y = m_bounding_rect.y + 94 - m_profile_rect.h / 2;
    }

    void player_view::set_hp_marker_position(float hp) {
        m_backup_characters.set_pos({
            m_characters.get_pos().x,
            m_characters.get_pos().y - std::max<int>(0, hp * options::one_hp_size)});
    }

    void player_view::set_gold(int amount) {
        gold = amount;

        if (amount > 0) {
            m_gold_text.redraw(std::to_string(amount));
        }
    }

    void player_view::render(sdl::renderer &renderer) {
        renderer.set_draw_color(sdl::rgb((border_color ? border_color : options::player_view_border_rgba) >> 8));
        renderer.draw_rect(m_bounding_rect);
        renderer.draw_rect(sdl::rect{
            m_bounding_rect.x + 1,
            m_bounding_rect.y + 1,
            m_bounding_rect.w - 2,
            m_bounding_rect.h - 2
        });

        m_role.render(renderer);
        if (!m_backup_characters.empty()) {
            m_backup_characters.front()->render(renderer);
            if (hp > 5) {
                sdl::rect hp_marker_rect = m_backup_characters.front()->get_rect();
                hp_marker_rect.y += options::one_hp_size * 5;
                card_textures::get().backface_character.render(renderer, hp_marker_rect);
            }
        }
        for (card_view *c : m_characters) {
            c->render(renderer);
        }
        if (gold > 0) {
            sdl::rect gold_rect = card_textures::get().icon_gold.get_rect();
            gold_rect.x = m_characters.get_pos().x - gold_rect.w / 2;
            gold_rect.y = m_characters.get_pos().y - options::gold_yoffset;
            card_textures::get().icon_gold.render(renderer, gold_rect);

            sdl::rect gold_text_rect = m_gold_text.get_rect();
            gold_text_rect.x = gold_rect.x + (gold_rect.w - gold_text_rect.w) / 2;
            gold_text_rect.y = gold_rect.y + (gold_rect.h - gold_text_rect.h) / 2;
            m_gold_text.set_rect(gold_text_rect);
            m_gold_text.render(renderer, false);
        }

        m_username_text.render(renderer);

        if (m_profile_image && *m_profile_image) {
            m_profile_image->render(renderer, m_profile_rect);
        }
    }
}