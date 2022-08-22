#include "player.h"

#include "../media_pak.h"
#include "game.h"

namespace banggame {
    using namespace sdl::point_math;

    player_view::player_view(game_scene *game, int id)
        : m_game(game)
        , id(id)
        , m_username_text(widgets::text_style{
            .text_font = &media_pak::font_bkant_bold
        })
    {}

    void player_view::set_hp_marker_position(float hp) {
        m_backup_characters.set_pos({
            m_characters.get_pos().x,
            m_characters.get_pos().y - std::max(0, static_cast<int>(hp * options.one_hp_size))});
    }

    void player_view::set_gold(int amount) {
        gold = amount;

        if (amount > 0) {
            m_gold_text.set_value(std::to_string(amount));
        }
    }

    void alive_player_view::set_position(sdl::point pos) {
        m_bounding_rect.w = table.width + options.card_width * 3 + options.card_margin * 4;
        m_bounding_rect.h = options.player_view_height;

        m_bounding_rect.x = pos.x - m_bounding_rect.w / 2;
        m_bounding_rect.y = pos.y - m_bounding_rect.h / 2;
        
        table.set_pos(sdl::point{
            m_bounding_rect.x + (table.width + options.card_width) / 2 + options.card_margin,
            m_bounding_rect.y + m_bounding_rect.h / 2});
        hand.set_pos(table.get_pos());
        m_characters.set_pos(sdl::point{
            m_bounding_rect.x + m_bounding_rect.w - options.card_width - options.card_width / 2 - options.card_margin * 2,
            m_bounding_rect.y + m_bounding_rect.h - options.card_width - options.card_margin});
        m_role->set_pos(sdl::point(
            m_characters.get_pos().x + options.card_width + options.card_margin,
            m_characters.get_pos().y + options.role_yoff));
        set_hp_marker_position(static_cast<float>(hp));
        if (this == m_game->m_player_self) {
            hand.set_pos(hand.get_pos() + sdl::point{0, options.card_yoffset});
            table.set_pos(table.get_pos() - sdl::point{0, options.card_yoffset});
        } else {
            table.set_pos(table.get_pos() + sdl::point{0, options.card_yoffset});
            hand.set_pos(hand.get_pos() - sdl::point{0, options.card_yoffset});
        }

        scenario_deck.set_pos(m_role->get_pos() + sdl::point{options.character_offset, options.character_offset});

        m_propic.set_pos(sdl::point{
            m_role->get_pos().x,
            m_bounding_rect.y + options.propic_yoff
        });

        set_username(m_username_text.get_value());
    }

    void alive_player_view::set_username(const std::string &value) {
        m_username_text.set_value(value);
        sdl::rect username_rect = m_username_text.get_rect();
        username_rect.x = m_role->get_pos().x - (username_rect.w) / 2;
        username_rect.y = m_bounding_rect.y + options.username_yoff;
        m_username_text.set_rect(username_rect);
    }

    void alive_player_view::render(sdl::renderer &renderer) {
        renderer.set_draw_color(border_color.a ? border_color : sdl::full_alpha(options.player_view_border));
        renderer.draw_rect(m_bounding_rect);
        renderer.draw_rect(sdl::rect{
            m_bounding_rect.x + 1,
            m_bounding_rect.y + 1,
            m_bounding_rect.w - 2,
            m_bounding_rect.h - 2
        });

        m_role->render(renderer);
        if (!m_backup_characters.empty()) {
            m_backup_characters.front()->render(renderer);
            if (hp > 5) {
                sdl::rect hp_marker_rect = m_backup_characters.front()->get_rect();
                hp_marker_rect.y += options.one_hp_size * 5;
                card_textures::get().backfaces[enums::indexof(card_deck_type::character)].render(renderer, hp_marker_rect);
            }
        }
        for (card_view *c : m_characters) {
            c->render(renderer);
        }
        if (gold > 0) {
            sdl::rect gold_rect = media_pak::get().icon_gold.get_rect();
            gold_rect.x = m_characters.get_pos().x - gold_rect.w / 2;
            gold_rect.y = m_characters.get_pos().y - options.gold_yoffset;
            media_pak::get().icon_gold.render(renderer, gold_rect);

            sdl::rect gold_text_rect = m_gold_text.get_rect();
            gold_text_rect.x = gold_rect.x + (gold_rect.w - gold_text_rect.w) / 2;
            gold_text_rect.y = gold_rect.y + (gold_rect.h - gold_text_rect.h) / 2;
            m_gold_text.set_rect(gold_text_rect);
            m_gold_text.render(renderer, false);
        }

        m_username_text.render(renderer);

        m_propic.render(renderer);

        table.render(renderer);
        hand.render(renderer);

        scenario_deck.render_last(renderer, 1);

        int x = m_bounding_rect.x + m_bounding_rect.w - 5;

        auto render_icon = [&](sdl::texture_ref texture, sdl::color color = sdl::rgba(0xffffffff)) {
            sdl::rect rect = texture.get_rect();
            rect.x = x - rect.w;
            rect.y = m_bounding_rect.y + 5;
            texture.render_colored(renderer, rect, color);
        };

        if (m_game->m_winner_role == player_role::unknown) {
            if (this == m_game->m_playing) {
                render_icon(media_pak::get().icon_turn, options.turn_indicator);
                x -= 32;
            }
            if (this == m_game->m_request_target) {
                render_icon(media_pak::get().icon_target, options.request_target_indicator);
                x -= 32;
            }
            if (this == m_game->m_request_origin) {
                render_icon(media_pak::get().icon_origin, options.request_origin_indicator);
            }
        } else if (m_role->role == m_game->m_winner_role
            || (m_role->role == player_role::deputy && m_game->m_winner_role == player_role::sheriff)
            || (m_role->role == player_role::sheriff && m_game->m_winner_role == player_role::deputy)) {
            render_icon(media_pak::get().icon_winner, options.winner_indicator);
        }
    }

    void dead_player_view::set_position(sdl::point pos) {
        m_bounding_rect.w = options.card_width + options.card_margin + widgets::profile_pic::size;
        m_bounding_rect.h = options.pile_dead_players_card_ydiff;
        m_bounding_rect.x = pos.x;
        m_bounding_rect.y = pos.y;

        m_role->set_pos(sdl::point{
            m_bounding_rect.x + m_bounding_rect.w - options.card_width / 2,
            m_bounding_rect.y
        });

        m_propic.set_pos(sdl::point{
            m_bounding_rect.x + widgets::profile_pic::size / 2,
            m_bounding_rect.y + options.dead_propic_yoff
        });

        set_username(m_username_text.get_value());
    }

    void dead_player_view::set_username(const std::string &value) {
        m_username_text.set_value(value);
        sdl::rect username_rect = m_username_text.get_rect();
        username_rect.x = m_propic.get_pos().x - username_rect.w / 2;
        username_rect.y = m_propic.get_pos().y + options.username_yoff - options.propic_yoff;
        m_username_text.set_rect(username_rect);
    }

    void dead_player_view::render(sdl::renderer &renderer) {
        m_role->render(renderer);
        m_propic.render(renderer);
        m_username_text.render(renderer);

        if (m_role->role == m_game->m_winner_role
            || (m_role->role == player_role::deputy && m_game->m_winner_role == player_role::sheriff)
            || (m_role->role == player_role::sheriff && m_game->m_winner_role == player_role::deputy)) {
            
            sdl::texture_ref texture = media_pak::get().icon_winner;
            sdl::rect rect = texture.get_rect();
            rect.x = m_propic.get_pos().x - rect.w - widgets::profile_pic::size / 2 - 5;
            rect.y = m_propic.get_pos().y - rect.h / 2;
            texture.render_colored(renderer, rect, options.winner_indicator);
        }
    }
}