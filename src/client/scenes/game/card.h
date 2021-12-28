#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"
#include "common/game_update.h"

#include "common/sdl.h"
#include "sizes.h"
#include "../widgets/stattext.h"

#include <vector>
#include <list>

namespace banggame {

    struct card_textures {
    private:
        static inline sdl::surface s_card_mask;

        static inline sdl::texture s_main_deck;
        static inline sdl::texture s_character;
        static inline sdl::texture s_role;
        static inline sdl::texture s_goldrush;

        static inline int s_counter = 0;

    public:
        card_textures();
        ~card_textures();

        static sdl::surface apply_card_mask(const sdl::surface &source);
        
        static const sdl::texture &main_deck() { return s_main_deck; }
        static const sdl::texture &character() { return s_character; }
        static const sdl::texture &role() { return s_role; }
        static const sdl::texture &goldrush() { return s_goldrush; }
    };

    struct card_widget;
    struct card_view;

    struct card_pile_view : std::vector<card_view *> {
        sdl::point pos;
        int width;
        int hflip;

        explicit card_pile_view(int width, bool hflip = false)
            : width(width)
            , hflip(hflip ? -1 : 1) {}

        sdl::point get_position(card_view *card) const {
            if (size() == 1) {
                return pos;
            }
            float xoffset = std::min(float(width) / (size() - 1), float(sizes::card_width + sizes::card_xoffset)) * hflip;

            return sdl::point{(int)(pos.x + xoffset *
                (std::ranges::distance(begin(), std::ranges::find(*this, card)) - (size() - 1) * .5f)),
                pos.y};
        }

        void erase_card(card_view *card) {
            if (auto it = std::ranges::find(*this, card); it != end()) {
                erase(it);
            }
        }
    };

    class cube_widget {
    public:
        card_widget *owner = nullptr;

        int id;
        sdl::point pos;

        bool animating = false;

        cube_widget(int id) : id(id) {}

        void render(sdl::renderer &renderer, bool skip_if_animating = true);

        const sdl::rect get_rect() const { return m_rect; }

    private:
        static sdl::texture cube_texture;
        sdl::rect m_rect;
    };

    class card_widget : public card_info {
    public:
        std::vector<cube_widget *> cubes;

        float flip_amt = 0.f;
        float rotation = 0.f;

        bool animating = false;
        
        bool known = false;
        card_pile_view *pile = nullptr;

        card_suit_type suit = card_suit_type::none;
        card_value_type value = card_value_type::none;

        void set_pos(const sdl::point &pos);
        const sdl::point &get_pos() const {
            return m_pos;
        }

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void render(sdl::renderer &renderer, bool skip_if_animating = true) {
            if (!skip_if_animating || !animating) {
                if (flip_amt > 0.5f && texture_front_scaled) do_render(renderer, texture_front_scaled);
                else if (texture_back) do_render(renderer, *texture_back);
            }
        }

        sdl::texture texture_front;
        sdl::texture texture_front_scaled;

        const sdl::texture *texture_back = nullptr;

        void set_texture_front(sdl::texture &&tex) {
            texture_front = std::move(tex);
            texture_front_scaled = sdl::scale_surface(texture_front.get_surface(),
                texture_front.get_rect().w / sizes::card_width);
        }

    private:
        sdl::point m_pos;
        sdl::rect m_rect;
        
        void do_render(sdl::renderer &renderer, const sdl::texture &front);
    };

    struct card_view : card_widget {
        bool inactive = false;

        card_color_type color;

        void make_texture_front();
    };

    struct character_card : card_widget {
        void make_texture_front();
    };

    struct role_card : card_widget {
        player_role role = player_role::unknown;

        void make_texture_front();
    };

    struct player_view {
        int id;

        int user_id;
        
        explicit player_view(int id) : id(id) {}

        int hp = 0;
        int gold = 0;
        bool dead = false;

        card_pile_view hand{sizes::player_hand_width};
        card_pile_view table{sizes::player_hand_width};

        sdl::rect m_bounding_rect;

        std::list<character_card> m_characters{1};
        
        sdl::point m_hp_marker_pos;
        role_card m_role;

        sdl::stattext m_username_text;

        sdl::stattext m_gold_text;

        sdl::texture *m_profile_image = nullptr;
        sdl::rect m_profile_rect;
        
        static inline sdl::texture m_gold_texture;

        void set_position(sdl::point pos, bool flipped = false);

        void set_hp_marker_position(float hp);

        void set_gold(int amount);

        void set_username(const std::string &name);
        void set_profile_image(sdl::texture *image);

        void render(sdl::renderer &renderer);
        void render_turn_indicator(sdl::renderer &renderer);
        void render_request_origin_indicator(sdl::renderer &renderer);
        void render_request_target_indicator(sdl::renderer &renderer);
    };
}

#endif