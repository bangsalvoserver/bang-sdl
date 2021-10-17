#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"
#include "common/game_update.h"

#include "utils/sdl.h"
#include "sizes.h"
#include "../widgets/stattext.h"

#include <vector>
#include <list>

namespace banggame {

    namespace textures_back {
        sdl::texture &main_deck();
        sdl::texture &character();
        sdl::texture &role();
        sdl::texture &goldrush();
    };

    struct card_pile_view : std::vector<int> {
        sdl::point pos;
        int width;

        explicit card_pile_view(int width) : width(width) {}

        auto find(int card_id) const {
            return std::ranges::find(*this, card_id);
        }

        sdl::point get_position(int card_id) const {
            if (size() == 1) {
                return pos;
            }
            float xoffset = std::min((float)width / (size() - 1), (float)(sizes::card_width + sizes::card_xoffset));
            return sdl::point{(int)(pos.x + xoffset *
                (std::ranges::distance(begin(), find(card_id)) - (size() - 1) * .5f)),
                pos.y};
        }

        void erase_card(int card_id) {
            if (auto it = find(card_id); it != end()) {
                erase(it);
            }
        }
    };

    class card_widget : public card_info {
    public:
        sdl::point pos;

        float flip_amt = 0.f;
        float rotation = 0.f;

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void render(sdl::renderer &renderer) {
            if (flip_amt > 0.5f && texture_front_scaled) do_render(renderer, texture_front_scaled);
            else if (texture_back) do_render(renderer, *texture_back);
        }

        sdl::texture texture_front;
        sdl::texture texture_front_scaled;

        sdl::texture *texture_back = nullptr;

        void set_texture_front(sdl::texture &&tex) {
            texture_front = std::move(tex);
            texture_front_scaled = sdl::scale_surface(texture_front.get_surface(),
                texture_front.get_rect().w / sizes::card_width);
        }

    private:
        sdl::rect m_rect;
        
        void do_render(sdl::renderer &renderer, sdl::texture &front);
    };

    struct card_view : card_widget {
        bool known = false;
        bool inactive = false;

        card_pile_view *pile = nullptr;

        card_suit_type suit;
        card_value_type value;
        card_color_type color;

        void make_texture_front();
    };

    struct character_card : card_widget {
        character_type type;

        void make_texture_front();
    };

    struct role_card : card_widget {
        player_role role = player_role::unknown;

        void make_texture_front();
    };

    struct player_view {
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
        
        static inline sdl::texture m_gold_texture;

        void set_position(sdl::point pos, bool flipped = false);

        void set_hp_marker_position(float hp);

        void set_gold(int amount);

        void render(sdl::renderer &renderer);
        void render_turn_indicator(sdl::renderer &renderer);
        void render_request_indicator(sdl::renderer &renderer);
    };
}

#endif