#include "card.h"

#include "common/unpacker.h"

#include <SDL2/SDL.h>

static std::ifstream cards_pak_data(std::string(SDL_GetBasePath()) + "cards.pak", std::ios::in | std::ios::binary);
static const unpacker card_resources(cards_pak_data);

namespace banggame {

    card_textures::card_textures() {
        if (s_counter++ == 0) {
            s_card_mask = sdl::surface(card_resources["mask"]);

            s_main_deck = apply_card_mask(sdl::surface(card_resources["back_card"]));
            s_character = apply_card_mask(sdl::surface(card_resources["back_character"]));
            s_role = apply_card_mask(sdl::surface(card_resources["back_role"]));
            s_goldrush = apply_card_mask(sdl::surface(card_resources["back_goldrush"]));
        }
    }

    card_textures::~card_textures() {
        if (--s_counter == 0) {
            s_card_mask.clear();
            s_main_deck.clear();
            s_character.clear();
            s_role.clear();
            s_goldrush.clear();
        }
    }
    
    sdl::surface card_textures::apply_card_mask(const sdl::surface &source) {
        sdl::surface ret(s_card_mask.get()->w, s_card_mask.get()->h);
        sdl::rect src_rect = source.get_rect();
        sdl::rect dst_rect = ret.get_rect();
        SDL_BlitSurface(source.get(), &src_rect, ret.get(), &dst_rect);

        SDL_LockSurface(s_card_mask.get());
        SDL_LockSurface(ret.get());

        const uint32_t amask = s_card_mask.get()->format->Amask;

        uint32_t *mask_ptr = static_cast<uint32_t *>(s_card_mask.get()->pixels);
        uint32_t *surf_ptr = static_cast<uint32_t *>(ret.get()->pixels);

        int npixels = ret.get()->w * ret.get()->h;

        for (int i=0; i<npixels; ++i) {
            *surf_ptr = (*surf_ptr & (~amask)) | (*mask_ptr & amask);
            ++mask_ptr;
            ++surf_ptr;
        }

        SDL_UnlockSurface(ret.get());
        SDL_UnlockSurface(s_card_mask.get());

        return ret;
    }

    void card_view::make_texture_front() {
        sdl::surface card_base_surf(card_resources[image]);
        sdl::rect card_rect = card_base_surf.get_rect();

        if (value != card_value_type::none) {
            sdl::surface card_value_surf(card_resources[enums::to_string(value)]);
            sdl::rect value_rect = card_value_surf.get_rect();

            value_rect.x = 15;
            value_rect.y = card_rect.h - value_rect.h - 15;
                
            SDL_BlitSurface(card_value_surf.get(), nullptr, card_base_surf.get(), &value_rect);
            
            if (suit != card_suit_type::none) {
                std::string suit_string = "suit_";
                suit_string.append(enums::to_string(suit));
                sdl::surface card_suit_surf(card_resources[suit_string]);
                
                sdl::rect suit_rect = card_suit_surf.get_rect();

                suit_rect.x = value_rect.x + value_rect.w;
                suit_rect.y = card_rect.h - suit_rect.h - 15;

                SDL_BlitSurface(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);
            }
        }

        set_texture_front(card_textures::apply_card_mask(card_base_surf));
    }

    void character_card::make_texture_front() {
        set_texture_front(card_textures::apply_card_mask(sdl::surface(card_resources[image])));
        flip_amt = 1.f;
    }

    void role_card::make_texture_front() {
        std::string role_string = "role_";
        role_string.append(enums::to_string(role));
        set_texture_front(card_textures::apply_card_mask(sdl::surface(card_resources[role_string])));
    }

    sdl::texture cube_widget::cube_texture{sdl::surface(card_resources["cube"])};

    void cube_widget::render(sdl::renderer &renderer, bool skip_if_animating) {
        if (!skip_if_animating || !animating) {
            m_rect = cube_texture.get_rect();
            m_rect.x = pos.x - m_rect.w / 2;
            m_rect.y = pos.y - m_rect.h / 2;
            cube_texture.render(renderer, m_rect);
        }
    }

    void card_widget::set_pos(const sdl::point &new_pos) {
        for (auto *cube : cubes) {
            sdl::point diff{cube->pos.x - m_pos.x, cube->pos.y - m_pos.y};
            cube->pos = sdl::point{diff.x + new_pos.x, diff.y + new_pos.y};
        }
        m_pos = new_pos;
    }
    
    void card_widget::do_render(sdl::renderer &renderer, const sdl::texture &tex) {
        m_rect = tex.get_rect();
        sdl::scale_rect(m_rect, sizes::card_width);
        
        m_rect.x = m_pos.x - m_rect.w / 2;
        m_rect.y = m_pos.y - m_rect.h / 2;

        sdl::rect rect = m_rect;
        float wscale = std::abs(1.f - 2.f * flip_amt);
        rect.x += rect.w * (1.f - wscale) * 0.5f;
        rect.w *= wscale;
        SDL_RenderCopyEx(renderer.get(), tex.get_texture(renderer), nullptr, &rect, rotation, nullptr, SDL_FLIP_NONE);

        for (auto *cube : cubes) {
            cube->render(renderer);
        }
    }

    void player_view::set_position(sdl::point pos, bool flipped) {
        m_bounding_rect.w = table.width + sizes::card_width * 3 + 40;
        m_bounding_rect.h = sizes::player_view_height;
        m_bounding_rect.x = pos.x - m_bounding_rect.w / 2;
        m_bounding_rect.y = pos.y - m_bounding_rect.h / 2;
        hand.pos = table.pos = sdl::point{
            m_bounding_rect.x + (table.width + sizes::card_width) / 2 + 10,
            m_bounding_rect.y + m_bounding_rect.h / 2};
        sdl::point char_pos = sdl::point{
            m_bounding_rect.x + m_bounding_rect.w - sizes::card_width - sizes::card_width / 2 - 20,
            m_bounding_rect.y + m_bounding_rect.h - sizes::card_width - 10};
        m_role.set_pos(sdl::point(char_pos.x + sizes::card_width + 10, char_pos.y));
        for (auto &c : m_characters) {
            c.set_pos(char_pos);
            char_pos.x += sizes::character_offset;
            char_pos.y += sizes::character_offset;
        }
        set_hp_marker_position(hp);
        if (flipped) {
            hand.pos.y += sizes::card_yoffset;
            table.pos.y -= sizes::card_yoffset;
        } else {
            hand.pos.y -= sizes::card_yoffset;
            table.pos.y += sizes::card_yoffset;
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
        username_rect.y = m_bounding_rect.y + 20;
        m_username_text.set_rect(username_rect);
    }

    void player_view::set_profile_image(sdl::texture *image) {
        m_profile_image = image;
        if (!image ||!*image) return;

        sdl::point profile_image_pos = sdl::point{
            (m_role.get_rect().y + m_username_text.get_rect().y + m_username_text.get_rect().h) / 2,
            m_role.get_pos().x
        };

        m_profile_rect = m_profile_image->get_rect();
        if (m_profile_rect.w > m_profile_rect.h) {
            m_profile_rect.h = m_profile_rect.h * sizes::propic_size / m_profile_rect.w;
            m_profile_rect.w = sizes::propic_size;
        } else {
            m_profile_rect.w = m_profile_rect.w * sizes::propic_size / m_profile_rect.h;
            m_profile_rect.h = sizes::propic_size;
        }

        m_profile_rect.x = m_role.get_pos().x - m_profile_rect.w / 2;
        m_profile_rect.y = m_bounding_rect.y + 70 - m_profile_rect.h / 2;
    }

    void player_view::set_hp_marker_position(float hp) {
        m_hp_marker_pos = m_characters.front().get_pos();
        m_hp_marker_pos.y -= std::max(0.f, hp * sizes::one_hp_size);
    }

    void player_view::set_gold(int amount) {
        gold = amount;

        if (!m_gold_texture) {
            m_gold_texture = sdl::surface(card_resources["gold"]);
        }

        if (amount > 0) {
            m_gold_text.redraw(std::to_string(amount));
        }
    }

    void player_view::render(sdl::renderer &renderer) {
        renderer.set_draw_color(sdl::rgba(sizes::player_view_border_rgba));
        renderer.draw_rect(m_bounding_rect);
        m_role.render(renderer);
        sdl::rect hp_marker_rect = m_characters.front().get_rect();
        hp_marker_rect.x = m_hp_marker_pos.x - hp_marker_rect.w / 2;
        hp_marker_rect.y = m_hp_marker_pos.y - hp_marker_rect.h / 2;
        card_textures::character().render(renderer, hp_marker_rect);
        if (hp > 5) {
            hp_marker_rect.y += sizes::one_hp_size * 5;
            card_textures::character().render(renderer, hp_marker_rect);
        }
        for (auto &c : m_characters) {
            c.render(renderer);
        }
        if (gold > 0 && m_gold_texture) {
            sdl::rect gold_rect = m_gold_texture.get_rect();
            gold_rect.x = m_characters.front().get_pos().x - gold_rect.w / 2;
            gold_rect.y = m_characters.front().get_pos().y - sizes::gold_yoffset;
            m_gold_texture.render(renderer, gold_rect);

            sdl::rect gold_text_rect = m_gold_text.get_rect();
            gold_text_rect.x = gold_rect.x + (gold_rect.w - gold_text_rect.w) / 2;
            gold_text_rect.y = gold_rect.y + (gold_rect.h - gold_text_rect.h) / 2;
            m_gold_text.set_rect(gold_text_rect);
            m_gold_text.render(renderer);
        }

        m_username_text.render(renderer);

        if (m_profile_image && *m_profile_image) {
            m_profile_image->render(renderer, m_profile_rect);
        }
    }

    inline void draw_border(sdl::renderer &renderer, const sdl::rect &rect, int border, const sdl::color &color) {
        renderer.set_draw_color(color);
        renderer.draw_rect(sdl::rect{
            rect.x - border,
            rect.y - border,
            rect.w + 2 * border,
            rect.h + 2 * border
        });
    }

    void player_view::render_turn_indicator(sdl::renderer &renderer) {
        draw_border(renderer, m_bounding_rect, sizes::turn_indicator_border, sdl::rgba(sizes::turn_indicator_rgba));
    }

    void player_view::render_request_origin_indicator(sdl::renderer &renderer) {
        draw_border(renderer, m_bounding_rect, sizes::request_origin_indicator_border, sdl::rgba(sizes::request_origin_indicator_rgba));
    }

    void player_view::render_request_target_indicator(sdl::renderer &renderer) {
        draw_border(renderer, m_bounding_rect, sizes::request_target_indicator_border, sdl::rgba(sizes::request_target_indicator_rgba));
    }
}