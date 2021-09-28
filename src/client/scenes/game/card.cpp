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
        SDL_Rect src_rect = source.get_rect();
        SDL_Rect dst_rect = ret.get_rect();
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
        SDL_Rect card_rect = card_base_surf.get_rect();

        sdl::surface card_value_surf(misc_resources[enums::to_string(value)]);
        SDL_Rect value_rect = card_value_surf.get_rect();

        value_rect.x = 15;
        value_rect.y = card_rect.h - value_rect.h - 15;
            
        SDL_BlitSurface(card_value_surf.get(), nullptr, card_base_surf.get(), &value_rect);

        std::string suit_string = "suit_";
        suit_string.append(enums::to_string(suit));
        sdl::surface card_suit_surf(misc_resources[suit_string]);
        
        SDL_Rect suit_rect = card_suit_surf.get_rect();

        suit_rect.x = value_rect.x + value_rect.w;
        suit_rect.y = card_rect.h - suit_rect.h - 15;

        SDL_BlitSurface(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);

        texture_front = apply_card_mask(card_base_surf);
    }

    void card_view::make_texture_back() {
        texture_back = apply_card_mask(sdl::surface(misc_resources["back_card"]));
    }

    void player_view::make_texture_character() {
        m_character.texture_front = apply_card_mask(sdl::surface(character_resources[image]));
        m_character.flip_amt = 1.f;
    }

    void player_view::make_texture_role() {
        std::string role_string = "role_";
        role_string.append(enums::to_string(role));
        m_role.texture_front = apply_card_mask(sdl::surface(misc_resources[role_string]));
        m_role.flip_amt = 1.f;
    }

    void player_view::make_textures_back() {
        character_card::texture_back = apply_card_mask(sdl::surface(misc_resources["back_character"]));
        role_card::texture_back = apply_card_mask(sdl::surface(misc_resources["back_role"]));
    }
    
    void card_widget_base::render(sdl::renderer &renderer, sdl::texture &tex) {
        SDL_Rect rect = tex.get_rect();
        rect.h = card_width * rect.h / rect.w;
        rect.w = card_width;

        rect.x = pos.x - rect.w / 2;
        rect.y = pos.y - rect.h / 2;

        float wscale = std::abs(1.f - 2.f * flip_amt);
        rect.x += rect.w * (1.f - wscale) * 0.5f;
        rect.w *= wscale;
        SDL_RenderCopyEx(renderer.get(), tex.get_texture(renderer), nullptr, &rect, rotation, nullptr, SDL_FLIP_NONE);
    }

    void player_view::set_position(SDL_Point pos, bool flipped) {
        hand.pos = table.pos = m_character.pos = m_role.pos = pos;
        m_character.pos.x += 200;
        set_hp_marker_position(hp);
        m_role.pos.x += 280;
        if (flipped) {
            hand.pos.y += 60;
            table.pos.y -= 60;
        } else {
            hand.pos.y -= 60;
            table.pos.y += 60;
        }
    }

    void player_view::set_hp_marker_position(float hp) {
        m_hp_marker.pos = m_character.pos;
        m_hp_marker.pos.y -= std::max(0.f, hp * 20.f);
    }

    void player_view::render(sdl::renderer &renderer) {
        m_role.render(renderer);
        m_hp_marker.render(renderer);
        m_character.render(renderer);
    }
}