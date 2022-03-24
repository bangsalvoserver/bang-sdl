#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "game/card_enums.h"
#include "game/game_update.h"

#include "utils/sdl.h"
#include "utils/unpacker.h"

#include "options.h"

#include "../widgets/stattext.h"

#include <filesystem>
#include <vector>
#include <list>

namespace banggame {

    struct card_textures {
    private:
        card_textures(const std::filesystem::path &base_path);

        static inline card_textures *s_instance = nullptr;

        std::ifstream cards_pak_data;
        const unpacker<std::ifstream> card_resources;

        friend class game_scene;

    public:
        sdl::surface card_mask;
        sdl::texture card_border;

        sdl::texture backface_maindeck;
        sdl::texture backface_character;
        sdl::texture backface_role;
        sdl::texture backface_goldrush;

        std::array<sdl::surface, enums::size_v<card_value_type> - 1> value_icons;
        std::array<sdl::surface, enums::size_v<card_suit_type> - 1> suit_icons;

        sdl::surface apply_card_mask(const sdl::surface &source) const;

        sdl::surface get_card_resource(std::string_view name) const {
            return sdl::surface(card_resources[name]);
        }

        static const card_textures &get() {
            return *s_instance;
        }
    };

    class card_pile_view;
    class cube_widget;

    class card_view : public card_data {
    public:
        std::vector<cube_widget *> cubes;

        float flip_amt = 0.f;
        float rotation = 0.f;

        bool animating = false;
        
        bool known = false;
        card_pile_view *pile = nullptr;

        bool inactive = false;

        sdl::color border_color{};

        void set_pos(const sdl::point &pos);
        const sdl::point &get_pos() const {
            return m_pos;
        }

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void render(sdl::renderer &renderer, bool skip_if_animating = true);

        sdl::texture texture_front;
        sdl::texture texture_front_scaled;

        const sdl::texture *texture_back = nullptr;
        
        void make_texture_front();

    private:
        sdl::point m_pos;
        sdl::rect m_rect;
    };

    class cube_widget {
    public:
        card_view *owner = nullptr;

        int id;
        sdl::point pos;

        bool animating = false;
        sdl::color border_color{};

        cube_widget(int id) : id(id) {}

        void render(sdl::renderer &renderer, bool skip_if_animating = true);
    };

    struct role_card : card_view {
        player_role role = player_role::unknown;

        void make_texture_front();
    };
    
    class card_pile_view {
    private:
        std::vector<card_view *> m_cards;
        sdl::point m_pos;

    public:
        sdl::color border_color{};

    public:
        sdl::point get_pos() const { return m_pos; }
        void set_pos(const sdl::point &pos);

        size_t size() const { return m_cards.size(); }
        bool empty() const { return m_cards.empty(); }

        auto begin() { return m_cards.begin(); }
        auto begin() const { return m_cards.begin(); }
        auto end() { return m_cards.end(); }
        auto end() const { return m_cards.end(); }
        auto rbegin() { return m_cards.rbegin(); }
        auto rbegin() const { return m_cards.rbegin(); }
        auto rend() { return m_cards.rend(); }
        auto rend() const { return m_cards.rend(); }

        auto &front() { return m_cards.front(); }
        const auto &front() const { return m_cards.front(); }
        auto &back() { return m_cards.back(); }
        const auto &back() const { return m_cards.back(); }
    
    public:
        virtual sdl::point get_position_of(card_view *card) const {
            return m_pos;
        }

        virtual bool wide() const {
            return false;
        }

        virtual void add_card(card_view *card) {
            card->pile = this;
            m_cards.push_back(card);
        }

        virtual void erase_card(card_view *card) {
            if (auto it = std::ranges::find(*this, card); it != end()) {
                card->pile = nullptr;
                m_cards.erase(it);
            }
        }

        virtual void clear() {
            for (card_view *card : *this) {
                if (card->pile == this) {
                    card->pile = nullptr;
                }
            }
            m_cards.clear();
        }
    };

    class counting_card_pile : public card_pile_view {
    private:
        widgets::stattext m_count_text;
        void update_count();

    public:
        void render_count(sdl::renderer &renderer);

        void add_card(card_view *card) override {
            card_pile_view::add_card(card);
            update_count();
        }

        void erase_card(card_view *card) override {
            card->pile = nullptr;
            card_pile_view::erase_card(card);
            update_count();
        }

        void clear() override {
            card_pile_view::clear();
            update_count();
        }
    };

    struct wide_card_pile : card_pile_view {
        int width;
        explicit wide_card_pile(int width) : width(width) {}

        bool wide() const override { return true; }
        sdl::point get_position_of(card_view *card) const override;
    };

    struct flipped_card_pile : wide_card_pile {
        using wide_card_pile::wide_card_pile;
        sdl::point get_position_of(card_view *card) const override;
    };

    struct character_pile : card_pile_view {
        sdl::point get_position_of(card_view *card) const override;
    };

    struct role_pile : card_pile_view {
        sdl::point get_position_of(card_view *card) const override;
    };

}

#endif