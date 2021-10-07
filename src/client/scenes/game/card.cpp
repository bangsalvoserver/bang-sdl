#include "card.h"

#include "utils/unpacker.h"

DECLARE_RESOURCE(cards_pak)
DECLARE_RESOURCE(misc_pak)
DECLARE_RESOURCE(characters_pak)

static const unpacker card_resources(GET_RESOURCE(cards_pak));
static const unpacker misc_resources(GET_RESOURCE(misc_pak));
static const unpacker character_resources(GET_RESOURCE(characters_pak));

namespace banggame {
    static const sdl::surface card_mask(misc_resources["mask"]);
    
    static sdl::surface apply_card_mask(const sdl::surface &source) {
        sdl::surface ret(card_mask.get()->w, card_mask.get()->h);
        sdl::rect src_rect = source.get_rect();
        sdl::rect dst_rect = ret.get_rect();
        SDL_BlitSurface(source.get(), &src_rect, ret.get(), &dst_rect);

        SDL_LockSurface(card_mask.get());
        SDL_LockSurface(ret.get());

        const uint32_t amask = card_mask.get()->format->Amask;

        uint32_t *mask_ptr = static_cast<uint32_t *>(card_mask.get()->pixels);
        uint32_t *surf_ptr = static_cast<uint32_t *>(ret.get()->pixels);

        int npixels = ret.get()->w * ret.get()->h;

        for (int i=0; i<npixels; ++i) {
            *surf_ptr = (*surf_ptr & (~amask)) | (*mask_ptr & amask);
            ++mask_ptr;
            ++surf_ptr;
        }

        SDL_UnlockSurface(ret.get());
        SDL_UnlockSurface(card_mask.get());

        return ret;
    }

    void card_view::make_texture_front() {
        sdl::surface card_base_surf(card_resources[image]);
        sdl::rect card_rect = card_base_surf.get_rect();

        sdl::surface card_value_surf(misc_resources[enums::to_string(value)]);
        sdl::rect value_rect = card_value_surf.get_rect();

        value_rect.x = 15;
        value_rect.y = card_rect.h - value_rect.h - 15;
            
        SDL_BlitSurface(card_value_surf.get(), nullptr, card_base_surf.get(), &value_rect);

        std::string suit_string = "suit_";
        suit_string.append(enums::to_string(suit));
        sdl::surface card_suit_surf(misc_resources[suit_string]);
        
        sdl::rect suit_rect = card_suit_surf.get_rect();

        suit_rect.x = value_rect.x + value_rect.w;
        suit_rect.y = card_rect.h - suit_rect.h - 15;

        SDL_BlitSurface(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);

        texture_front = apply_card_mask(card_base_surf);
    }

    void card_view::make_texture_back() {
        texture_back = apply_card_mask(sdl::surface(misc_resources["back_card"]));
    }

    void character_card::make_texture_front() {
        texture_front = apply_card_mask(sdl::surface(character_resources[image]));
        flip_amt = 1.f;
    }

    void character_card::make_texture_back() {
        texture_back = apply_card_mask(sdl::surface(misc_resources["back_character"]));
    }

    void role_card::make_texture_front() {
        std::string role_string = "role_";
        role_string.append(enums::to_string(role));
        texture_front = apply_card_mask(sdl::surface(misc_resources[role_string]));
    }

    void role_card::make_texture_back() {
        texture_back = apply_card_mask(sdl::surface(misc_resources["back_role"]));
    }
    
    void card_widget_base::render(sdl::renderer &renderer, sdl::texture &tex) {
        m_rect = tex.get_rect();
        sdl::scale_rect(m_rect, card_width);
        
        m_rect.x = pos.x - m_rect.w / 2;
        m_rect.y = pos.y - m_rect.h / 2;

        sdl::rect rect = m_rect;
        float wscale = std::abs(1.f - 2.f * flip_amt);
        rect.x += rect.w * (1.f - wscale) * 0.5f;
        rect.w *= wscale;
        SDL_RenderCopyEx(renderer.get(), tex.get_texture(renderer), nullptr, &rect, rotation, nullptr, SDL_FLIP_NONE);
    }

    void player_view::set_position(sdl::point pos, bool flipped) {
        m_bounding_rect.w = table.width + card_widget_base::card_width * 3 + 40;
        m_bounding_rect.h = 250;
        m_bounding_rect.x = pos.x - m_bounding_rect.w / 2;
        m_bounding_rect.y = pos.y - m_bounding_rect.h / 2;
        hand.pos = table.pos = sdl::point{
            m_bounding_rect.x + (table.width + card_widget_base::card_width) / 2 + 10,
            m_bounding_rect.y + m_bounding_rect.h / 2};
        sdl::point char_pos = sdl::point{
            m_bounding_rect.x + m_bounding_rect.w - card_widget_base::card_width - card_widget_base::card_width / 2 - 20,
            m_bounding_rect.y + m_bounding_rect.h - card_widget_base::card_width - 10};
        m_role.pos = sdl::point(char_pos.x + card_widget_base::card_width + 10, char_pos.y);
        for (auto &c : m_characters) {
            c.pos = char_pos;
            char_pos.x += 20;
            char_pos.y += 20;
        }
        set_hp_marker_position(hp);
        if (flipped) {
            hand.pos.y += 60;
            table.pos.y -= 60;
        } else {
            hand.pos.y -= 60;
            table.pos.y += 60;
        }
    }

    void player_view::set_hp_marker_position(float hp) {
        m_hp_marker.pos = m_characters.front().pos;
        m_hp_marker.pos.y -= std::max(0.f, hp * one_hp_size);
    }

    void player_view::render(sdl::renderer &renderer) {
        renderer.set_draw_color(sdl::color{0x0, 0x0, 0x0, 0xff});
        renderer.draw_rect(m_bounding_rect);
        m_role.render(renderer);
        m_hp_marker.render(renderer);
        if (hp > 5) {
            auto rect = m_hp_marker.get_rect();
            rect.y += one_hp_size * 5;
            m_hp_marker.texture_back.render(renderer, rect);
        }
        for (auto &c : m_characters) {
            c.render(renderer);
        }
    }

    void player_view::render_turn_indicator(sdl::renderer &renderer) {
        constexpr int border = 5;
        renderer.set_draw_color(sdl::color{0xff, 0x0, 0x0, 0xff});
        renderer.draw_rect(sdl::rect{m_bounding_rect.x - border, m_bounding_rect.y - border, m_bounding_rect.w + border * 2, m_bounding_rect.h + border * 2});
    }

    void player_view::render_request_indicator(sdl::renderer &renderer) {
        constexpr int border = 10;
        renderer.set_draw_color(sdl::color{0x0, 0x0, 0xff, 0xff});
        renderer.draw_rect(sdl::rect{m_bounding_rect.x - border, m_bounding_rect.y - border, m_bounding_rect.w + border * 2, m_bounding_rect.h + border * 2});
    }
}