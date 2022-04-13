#include "card.h"

#include "server/net_options.h"
#include "../media_pak.h"

#include <fmt/format.h>

namespace banggame {

    template<typename T>
    concept first_is_none = requires {
        requires enums::reflected_enum<T>;
        T::none;
        requires static_cast<int>(T::none) == 0;
    };

    template<typename ESeq> struct remove_first{};

    template<first_is_none auto None, first_is_none auto ... Es>
    struct remove_first<enums::enum_sequence<None, Es ...>> : enums::enum_sequence<Es ...> {};

    template<first_is_none T>
    struct skip_none : remove_first<enums::make_enum_sequence<T>> {};

    card_textures::card_textures(const std::filesystem::path &base_path)
        : cards_pak_data(ifstream_or_throw(base_path / "cards.pak"))
        , card_resources(cards_pak_data)

        , card_mask             (get_card_resource("card_mask"))
        , card_border           (get_card_resource("card_border"))

        , backface_maindeck     (apply_card_mask(get_card_resource("back_maindeck")))
        , backface_character    (apply_card_mask(get_card_resource("back_character")))
        , backface_role         (apply_card_mask(get_card_resource("back_role")))
        , backface_goldrush     (apply_card_mask(get_card_resource("back_goldrush")))

        , rank_icons([&]<card_rank ... Es>(enums::enum_sequence<Es ...>) {
            return std::array {
                get_card_resource(enums::to_string(Es)) ...
            };
        }(skip_none<card_rank>()))

        , suit_icons([&]<card_suit ... Es>(enums::enum_sequence<Es ...>) {
            return std::array {
                get_card_resource(fmt::format("suit_{}", enums::to_string(Es))) ...
            };
        }(skip_none<card_suit>()))
    {
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
        auto do_make_texture = [&](float scale) {
            auto card_base_surf = card_textures::get().get_card_resource(image);

            if (sign) {
                sdl::rect card_rect = card_base_surf.get_rect();

                const auto &card_rank_surf = card_textures::get().rank_icons[enums::indexof(sign.rank) - 1];
                sdl::rect rank_rect = card_rank_surf.get_rect();

                rank_rect.w *= scale;
                rank_rect.h *= scale;
                rank_rect.x = options.card_suit_offset;
                rank_rect.y = card_rect.h - rank_rect.h - options.card_suit_offset;
                    
                SDL_BlitScaled(card_rank_surf.get(), nullptr, card_base_surf.get(), &rank_rect);
                
                const auto &card_suit_surf = card_textures::get().suit_icons[enums::indexof(sign.suit) - 1];
                sdl::rect suit_rect = card_suit_surf.get_rect();

                suit_rect.w *= scale;
                suit_rect.h *= scale;
                suit_rect.x = rank_rect.x + rank_rect.w;
                suit_rect.y = card_rect.h - suit_rect.h - options.card_suit_offset;

                SDL_BlitScaled(card_suit_surf.get(), nullptr, card_base_surf.get(), &suit_rect);
            }

            return card_base_surf;
        };

        texture_front = card_textures::get().apply_card_mask(do_make_texture(1.f));

        sdl::surface scaled = card_textures::get().apply_card_mask(do_make_texture(options.card_suit_scale));
        texture_front_scaled = sdl::scale_surface(scaled, scaled.get_rect().w / options.card_width);
    }

    void role_card::make_texture_front() {
        std::string role_string = "role_";
        role_string.append(enums::to_string(role));
        
        texture_front = card_textures::get().apply_card_mask(card_textures::get().get_card_resource(role_string));
        texture_front_scaled = sdl::scale_surface(texture_front.get_surface(),
            texture_front.get_rect().w / options.card_width);
    }

    void cube_widget::render(sdl::renderer &renderer, bool skip_if_animating) {
        auto do_render = [&](const sdl::texture &tex, sdl::color color = sdl::rgb(0xffffff)) {
            sdl::rect rect = tex.get_rect();
            rect.x = pos.x - rect.w / 2;
            rect.y = pos.y - rect.h / 2;
            tex.render_colored(renderer, rect, color);
        };

        if (!skip_if_animating || !animating) {
            if (border_color.a) {
                do_render(media_pak::get().sprite_cube_border, border_color);
            }
            do_render(media_pak::get().sprite_cube);
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
            sdl::scale_rect_width(m_rect, options.card_width);

            m_rect.x = m_pos.x - m_rect.w / 2;
            m_rect.y = m_pos.y - m_rect.h / 2;

            sdl::rect rect = m_rect;
            float wscale = std::abs(1.f - 2.f * flip_amt);
            rect.x += rect.w * (1.f - wscale) * 0.5f;
            rect.w *= wscale;

            if (border_color.a) {
                card_textures::get().card_border.render_colored(renderer, sdl::rect{
                    rect.x - options.default_border_thickness,
                    rect.y - options.default_border_thickness,
                    rect.w + options.default_border_thickness * 2,
                    rect.h + options.default_border_thickness * 2
                }, border_color);
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

    void pocket_view::set_pos(const sdl::point &pos) {
        for (card_view *c : *this) {
            int dx = c->get_pos().x - m_pos.x;
            int dy = c->get_pos().y - m_pos.y;
            c->set_pos(sdl::point{pos.x + dx, pos.y + dy});
        }
        m_pos = pos;
    }

    sdl::point wide_pocket::get_position_of(card_view *card) const {
        if (size() == 1) {
            return get_pos();
        }
        float xoffset = std::min(float(width) / (size() - 1), float(options.card_width + options.card_xoffset));

        return sdl::point{(int)(get_pos().x + xoffset *
            (std::ranges::distance(begin(), std::ranges::find(*this, card)) - (size() - 1) * .5f)),
            get_pos().y};
    }

    sdl::point flipped_pocket::get_position_of(card_view *card) const {
        auto pt = wide_pocket::get_position_of(card);
        return {get_pos().x * 2 - pt.x, pt.y};
    }

    sdl::point character_pile::get_position_of(card_view *card) const {
        int diff = std::ranges::distance(begin(), std::ranges::find(*this, card));
        return sdl::point{get_pos().x + options.character_offset * diff, get_pos().y + options.character_offset * diff};
    }

    sdl::point role_pile::get_position_of(card_view *card) const {
        int diff = std::ranges::distance(begin(), std::ranges::find(*this, card));
        return sdl::point{get_pos().x, get_pos().y + options.card_yoffset * diff};
    }

    void counting_pocket::update_count() {
        m_count_text.set_value(std::to_string(size()));
    }

    void counting_pocket::render_count(sdl::renderer &renderer) {
        if (empty()) return;
        
        sdl::rect rect = m_count_text.get_rect();
        rect.x = get_pos().x - rect.w / 2;
        rect.y = get_pos().y - rect.h / 2;
        m_count_text.set_rect(rect);
        m_count_text.render(renderer);
    }
}