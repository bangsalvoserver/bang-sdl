#include "card.h"

#include "common/options.h"

namespace banggame {

    card_textures::card_textures(const std::filesystem::path &base_path)
        : cards_pak_data(base_path / "cards.pak", std::ios::in | std::ios::binary)
        , card_resources(cards_pak_data)
    {
        card_mask = get_card_resource("mask");

        backface_maindeck = apply_card_mask(get_card_resource("back_card"));
        backface_character = apply_card_mask(get_card_resource("back_character"));
        backface_role = apply_card_mask(get_card_resource("back_role"));
        backface_goldrush = apply_card_mask(get_card_resource("back_goldrush"));

        card_border = get_card_resource("card_border");

        cube_icon = get_card_resource("cube");
        cube_border = get_card_resource("cube_border");

        gold_icon = get_card_resource("gold");

        bullet_icon = get_card_resource("icon_bullet");
        bullet_border = get_card_resource("icon_bullet_border");

        s_instance = this;
    }
    
    sdl::surface card_textures::apply_card_mask(const sdl::surface &source) const {
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
        auto card_base_surf = card_textures::get().get_card_resource(image);

        if (value != card_value_type::none) {
            sdl::rect card_rect = card_base_surf.get_rect();
            auto card_value_surf = card_textures::get().get_card_resource(enums::to_string(value));
            sdl::rect value_rect = card_value_surf.get_rect();

            value_rect.x = 15;
            value_rect.y = card_rect.h - value_rect.h - 15;
                
            SDL_BlitSurface(card_value_surf.get(), nullptr, card_base_surf.get(), &value_rect);
            
            if (suit != card_suit_type::none) {
                std::string suit_string = "suit_";
                suit_string.append(enums::to_string(suit));
                auto card_suit_surf = card_textures::get().get_card_resource(suit_string);
                
                sdl::rect suit_rect = card_suit_surf.get_rect();

                suit_rect.x = value_rect.x + value_rect.w;
                suit_rect.y = card_rect.h - suit_rect.h - 15;

                SDL_BlitSurface(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);
            }
        }

        set_texture_front(card_textures::get().apply_card_mask(card_base_surf));
    }

    void role_card::make_texture_front() {
        std::string role_string = "role_";
        role_string.append(enums::to_string(role));
        set_texture_front(card_textures::get().apply_card_mask(card_textures::get().get_card_resource(role_string)));
    }

    void cube_widget::render(sdl::renderer &renderer, bool skip_if_animating) {
        auto do_render = [&](const sdl::texture &tex, sdl::color color = sdl::rgb(0xffffff)) {
            sdl::rect rect = tex.get_rect();
            rect.x = pos.x - rect.w / 2;
            rect.y = pos.y - rect.h / 2;
            tex.render_colored(renderer, rect, color);
        };

        if (!skip_if_animating || !animating) {
            if (border_color) {
                do_render(card_textures::get().cube_border, sdl::rgba(border_color));
            }
            do_render(card_textures::get().cube_icon);
        }
    }

    void card_view::set_pos(const sdl::point &new_pos) {
        for (auto *cube : cubes) {
            sdl::point diff{cube->pos.x - m_pos.x, cube->pos.y - m_pos.y};
            cube->pos = sdl::point{diff.x + new_pos.x, diff.y + new_pos.y};
        }
        m_pos = new_pos;
    }

    void card_view::render(sdl::renderer &renderer, bool skip_if_animating) {
        auto do_render = [&](const sdl::texture &tex) {
            m_rect = tex.get_rect();
            sdl::scale_rect(m_rect, sizes::card_width);

            m_rect.x = m_pos.x - m_rect.w / 2;
            m_rect.y = m_pos.y - m_rect.h / 2;

            sdl::rect rect = m_rect;
            float wscale = std::abs(1.f - 2.f * flip_amt);
            rect.x += rect.w * (1.f - wscale) * 0.5f;
            rect.w *= wscale;

            if (border_color) {
                card_textures::get().card_border.render_colored(renderer, sdl::rect{
                    rect.x - sizes::default_border_thickness,
                    rect.y - sizes::default_border_thickness,
                    rect.w + sizes::default_border_thickness * 2,
                    rect.h + sizes::default_border_thickness * 2
                }, sdl::rgba(border_color));
            }

            SDL_RenderCopyEx(renderer.get(), tex.get_texture(renderer), nullptr, &rect, rotation, nullptr, SDL_FLIP_NONE);

            for (auto *cube : cubes) {
                cube->render(renderer);
            }
        };

        if (!skip_if_animating || !animating) {
            if (flip_amt > 0.5f && texture_front_scaled) {
                do_render(texture_front_scaled);
            } else if (texture_back) {
                do_render(*texture_back);
            }
        }
    }

    void card_pile_view::set_pos(const sdl::point &new_pos) {
        for (card_view *c : *this) {
            int dx = c->get_pos().x - pos.x;
            int dy = c->get_pos().y - pos.y;
            c->set_pos(sdl::point{new_pos.x + dx, new_pos.y + dy});
        }
        pos = new_pos;
    }

    sdl::point card_pile_view::get_position_of(card_view *card) const {
        if (size() == 1 || m_width == 0) {
            return pos;
        }
        float xoffset = std::min(float(m_width) / (size() - 1), float(sizes::card_width + sizes::card_xoffset)) * hflip;

        return sdl::point{(int)(pos.x + xoffset *
            (std::ranges::distance(begin(), std::ranges::find(*this, card)) - (size() - 1) * .5f)),
            pos.y};
    }

    sdl::point character_pile::get_position_of(card_view *card) const {
        int diff = std::ranges::distance(begin(), std::ranges::find(*this, card));
        return sdl::point{pos.x + sizes::character_offset * diff, pos.y + sizes::character_offset * diff};
    }

    void card_pile_view::erase_card(card_view *card) {
        if (auto it = std::ranges::find(*this, card); it != end()) {
            erase(it);
        }
    }
}