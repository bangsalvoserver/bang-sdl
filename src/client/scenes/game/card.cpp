#include "card.h"

#include "utils/unpacker.h"

DECLARE_RESOURCE(cards_pak)
DECLARE_RESOURCE(misc_pak)

static const unpacker card_resources(GET_RESOURCE(cards_pak));
static const unpacker misc_resources(GET_RESOURCE(misc_pak));

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

    sdl::texture make_card_texture(const card_view &card) {
        sdl::surface card_base_surf(card_resources[card.image]);
        SDL_Rect card_rect = card_base_surf.get_rect();

        sdl::surface card_value_surf(misc_resources[enums::to_string(card.value)]);
        SDL_Rect value_rect = card_value_surf.get_rect();

        value_rect.x = 15;
        value_rect.y = card_rect.h - value_rect.h - 15;
            
        SDL_BlitSurface(card_value_surf.get(), nullptr, card_base_surf.get(), &value_rect);

        std::string suit_string = "suit_";
        suit_string.append(enums::to_string(card.suit));
        sdl::surface card_suit_surf(misc_resources[suit_string]);
        
        SDL_Rect suit_rect = card_suit_surf.get_rect();

        suit_rect.x = value_rect.x + value_rect.w;
        suit_rect.y = card_rect.h - suit_rect.h - 15;

        SDL_BlitSurface(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);
        
        return apply_card_mask(card_base_surf);
    }

    sdl::texture make_backface_texture() {
        return apply_card_mask(sdl::surface(misc_resources["back_card"]));
    }

    void card_view::render(sdl::renderer &renderer) {
        sdl::texture *tex = nullptr;
        if (flip_amt > 0.5f && texture_front) tex = &texture_front;
        else if (texture_back) tex = &texture_back;
        else return;
        
        SDL_Rect rect = tex->get_rect();
        rect.x = pos.x;
        rect.y = pos.y;
        scale_rect(rect, 70);

        rect.x -= rect.w / 2;
        rect.y -= rect.h / 2;

        float wscale = std::abs(1.f - 2.f * flip_amt);
        rect.x += rect.w * (1.f - wscale) * 0.5f;
        rect.w *= wscale;
        SDL_RenderCopyEx(renderer.get(), tex->get_texture(renderer), nullptr, &rect, rotation, nullptr, SDL_FLIP_NONE);
    }
}