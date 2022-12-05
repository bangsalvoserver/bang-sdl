#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "card.h"

#include "../widgets/text_list.h"
#include "../widgets/button.h"
#include "game_message_box.h"

#include <optional>

namespace banggame {

    class game_scene;

    class button_row_pocket : public pocket_view {
    public:
        game_scene *parent;
        std::list<widgets::button> m_buttons;

        button_row_pocket(game_scene *parent)
            : pocket_view(pocket_type::button_row)
            , parent(parent) {}

        void set_pos(const sdl::point &pos) override;
        void render(sdl::renderer &renderer) override;
        void add_card(card_view *card) override;
        void update_card(card_view *card) override;
        void erase_card(card_view *card) override;
        void clear() override;
    };

    class game_ui {
    public:
        game_ui(game_scene *parent);

        void refresh_layout();
        void render(sdl::renderer &renderer);

        void add_game_log(const std::string &message);

        void set_status(const std::string &message) {
            m_status_text.set_value(message);
        }
        void clear_status() {
            m_status_text = widgets::stattext();
        }
        void enable_golobby(bool value) {
            m_golobby_btn.set_enabled(value);
        }

        void show_message_box(const std::string &message, button_init_list &&buttons) {
            m_message_box.emplace(this, message, std::move(buttons));
            refresh_layout();
        }

        void close_message_box() {
            m_message_box.reset();
        }

        bool is_message_box_open() const {
            return m_message_box.has_value();
        }

    private:
        game_scene *parent;

        widgets::text_list m_game_log;

        widgets::stattext m_status_text;

        widgets::button m_leave_btn;
        widgets::button m_golobby_btn;

        widgets::button m_chat_btn;

        std::optional<game_message_box> m_message_box;
    };

}

#endif