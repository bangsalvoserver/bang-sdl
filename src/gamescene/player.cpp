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
        m_backup_characters.set_pos(m_characters.get_pos() - sdl::point{0, std::max(0, static_cast<int>(hp * options.one_hp_size))});
    }

    void player_view::set_gold(int amount) {
        gold = amount;

        if (amount > 0) {
            m_gold_text.set_value(std::to_string(amount));
        }
    }

    const player_view::player_state_vtable player_view::state_alive {
        .set_position = [](player_view *self, sdl::point pos) {
            self->m_bounding_rect = sdl::move_rect_center(sdl::rect{0, 0,
                options.card_pocket_width + options.card_width * 3 + options.card_margin * 4,
                options.player_view_height
            }, pos);
            
            sdl::point pockets_pos {
                self->m_bounding_rect.x + (options.card_pocket_width + options.card_width) / 2 + options.card_margin,
                self->m_bounding_rect.y + self->m_bounding_rect.h / 2
            };
            
            if (self == self->m_game->m_player_self) {
                self->hand.set_pos(pockets_pos + sdl::point{0, options.card_pocket_yoff});
                self->table.set_pos(pockets_pos - sdl::point{0, options.card_pocket_yoff});
            } else {
                self->table.set_pos(pockets_pos + sdl::point{0, options.card_pocket_yoff});
                self->hand.set_pos(pockets_pos - sdl::point{0, options.card_pocket_yoff});
            }

            self->m_characters.set_pos(sdl::point{
                self->m_bounding_rect.x + self->m_bounding_rect.w - options.card_width - options.card_width / 2 - options.card_margin * 2,
                self->m_bounding_rect.y + self->m_bounding_rect.h - options.card_width - options.card_margin
            });

            self->set_hp_marker_position(static_cast<float>(self->hp));

            self->m_role->set_pos(sdl::point{
                self->m_characters.get_pos().x + options.card_width + options.card_margin,
                self->m_characters.get_pos().y + options.role_yoff
            });

            self->scenario_deck.set_pos(self->m_role->get_pos() + sdl::point{options.character_offset, options.character_offset});

            self->m_propic.set_pos(sdl::point{
                self->m_role->get_pos().x,
                self->m_bounding_rect.y + options.propic_yoff
            });

            self->m_username_text.set_rect(sdl::move_rect_center(
                self->m_username_text.get_rect(),
                self->m_propic.get_pos() - sdl::point{0, options.username_yoff}
            ));
        },

        .render = [](player_view *self, sdl::renderer &renderer) {
            renderer.set_draw_color(self->border_color.a ? self->border_color : sdl::full_alpha(options.player_view_border));
            renderer.draw_rect(self->m_bounding_rect);
            renderer.draw_rect(sdl::rect{
                self->m_bounding_rect.x + 1,
                self->m_bounding_rect.y + 1,
                self->m_bounding_rect.w - 2,
                self->m_bounding_rect.h - 2
            });

            self->m_role->render(renderer);
            if (!self->m_backup_characters.empty()) {
                self->m_backup_characters.front()->render(renderer);
                if (self->hp > 5) {
                    sdl::rect hp_marker_rect = self->m_backup_characters.front()->get_rect();
                    hp_marker_rect.y += options.one_hp_size * 5;
                    card_textures::get().backfaces[enums::indexof(card_deck_type::character)].render(renderer, hp_marker_rect);
                }
            }
            for (card_view *c : self->m_characters) {
                c->render(renderer);
            }
            if (self->gold > 0) {
                sdl::rect gold_rect = media_pak::get().icon_gold.get_rect();
                gold_rect.x = self->m_characters.get_pos().x - gold_rect.w / 2;
                gold_rect.y = self->m_characters.get_pos().y - options.gold_yoffset;
                media_pak::get().icon_gold.render(renderer, gold_rect);

                sdl::rect gold_text_rect = self->m_gold_text.get_rect();
                gold_text_rect.x = gold_rect.x + (gold_rect.w - gold_text_rect.w) / 2;
                gold_text_rect.y = gold_rect.y + (gold_rect.h - gold_text_rect.h) / 2;
                self->m_gold_text.set_rect(gold_text_rect);
                self->m_gold_text.render(renderer, false);
            }

            self->m_username_text.render(renderer);

            self->m_propic.render(renderer);

            self->table.render(renderer);
            self->hand.render(renderer);

            self->scenario_deck.render_last(renderer, 1);

            int x = self->m_bounding_rect.x + self->m_bounding_rect.w - 5;

            auto render_icon = [&](sdl::texture_ref texture, sdl::color color = sdl::rgba(0xffffffff)) {
                sdl::rect rect = texture.get_rect();
                rect.x = x - rect.w;
                rect.y = self->m_bounding_rect.y + 5;
                texture.render_colored(renderer, rect, color);
            };

            if (self->m_game->m_winner_role == player_role::unknown) {
                if (self == self->m_game->m_playing) {
                    render_icon(media_pak::get().icon_turn, options.turn_indicator);
                    x -= 32;
                }
                if (self == self->m_game->m_request_target) {
                    render_icon(media_pak::get().icon_target, options.request_target_indicator);
                    x -= 32;
                }
                if (self == self->m_game->m_request_origin) {
                    render_icon(media_pak::get().icon_origin, options.request_origin_indicator);
                }
            } else if (self->m_role->role == self->m_game->m_winner_role
                || (self->m_role->role == player_role::deputy && self->m_game->m_winner_role == player_role::sheriff)
                || (self->m_role->role == player_role::sheriff && self->m_game->m_winner_role == player_role::deputy)) {
                render_icon(media_pak::get().icon_winner, options.winner_indicator);
            }
        }
    };

    const player_view::player_state_vtable player_view::state_dead {
        .set_position = [](player_view *self, sdl::point pos) {
            self->m_bounding_rect = sdl::move_rect_center(sdl::rect{0,0, options.card_width + options.card_margin + widgets::profile_pic::size, 0}, pos);

            self->m_role->set_pos(sdl::point{
                self->m_bounding_rect.x + self->m_bounding_rect.w - options.card_width / 2,
                self->m_bounding_rect.y
            });

            self->m_propic.set_pos(sdl::point{
                self->m_bounding_rect.x + widgets::profile_pic::size / 2,
                self->m_bounding_rect.y + options.dead_propic_yoff
            });

            self->m_username_text.set_rect(sdl::move_rect_center(
                self->m_username_text.get_rect(),
                self->m_propic.get_pos() - sdl::point{0, options.username_yoff}
            ));
        },

        .render = [](player_view *self, sdl::renderer &renderer) {
            self->m_role->render(renderer);
            self->m_propic.render(renderer);
            self->m_username_text.render(renderer);

            if (self->m_role->role == self->m_game->m_winner_role
                || (self->m_role->role == player_role::deputy && self->m_game->m_winner_role == player_role::sheriff)
                || (self->m_role->role == player_role::sheriff && self->m_game->m_winner_role == player_role::deputy)) {
                
                sdl::texture_ref texture = media_pak::get().icon_winner;
                sdl::rect rect = texture.get_rect();
                rect.x = self->m_propic.get_pos().x - rect.w - widgets::profile_pic::size / 2 - 5;
                rect.y = self->m_propic.get_pos().y - rect.h / 2;
                texture.render_colored(renderer, rect, options.winner_indicator);
            }
        }
    };
}