#ifndef __HOLDERS_H__
#define __HOLDERS_H__

#include <concepts>
#include <memory>

#include "utils/reflector.h"
#include "effects/card_effect.h"
#include "game_action.h"

namespace banggame {

    template<enums::reflected_enum E>
    struct effect_base {
        using enum_type = E;

        REFLECTABLE(
            (play_card_target_type) target,
            (target_player_filter) player_filter,
            (target_card_filter) card_filter,
            (short) effect_value,
            (enum_type) type
        )

        bool is(enum_type value) const { return type == value; }
    };

    struct effect_holder : effect_base<effect_type> {
        bool can_respond(card *origin_card, player *target) const;

        opt_error verify(card *origin_card, player *origin) const;
        opt_fmt_str on_prompt(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin, effect_flags flags);
        
        opt_error verify(card *origin_card, player *origin, player *target) const;
        opt_fmt_str on_prompt(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags);
        
        opt_error verify(card *origin_card, player *origin, card *target) const;
        opt_fmt_str on_prompt(card *origin_card, player *origin, card *target) const;
        void on_play(card *origin_card, player *origin, card *target, effect_flags flags);
    };
    
    struct equip_holder : effect_base<equip_type> {
        opt_fmt_str on_prompt(card *target_card, player *target) const;
        void on_equip(card *target_card, player *target);
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct mth_holder {
        REFLECTABLE((mth_type) type)
        
        opt_error verify(card *origin_card, player *origin, const target_list &targets) const;
        opt_fmt_str on_prompt(card *origin_card, player *origin, const target_list &targets) const;
        void on_play(card *origin_card, player *origin, const target_list &targets);
    };

    class request_holder {
    public:
        request_holder(std::shared_ptr<request_base> &&value)
            : m_value(std::move(value)) {}

        card *origin_card() const {
            return m_value->origin_card;
        }
        player *origin() const {
            return m_value->origin;
        }
        player *target() const {
            return m_value->target;
        }
        effect_flags flags() const {
            return m_value->flags;
        }
        game_formatted_string status_text(player *owner) const {
            return m_value->status_text(owner);
        }

        bool can_pick(pocket_type pocket, player *target, card *target_card) const {
            return m_value->can_pick(pocket, target, target_card);
        }
        void on_pick(pocket_type pocket, player *target, card *target_card) {
            auto copy = m_value;
            copy->on_pick(pocket, target, target_card);
        }

        void tick() {
            m_value->tick();
        }

        template<typename T> auto &get() {
            return dynamic_cast<T &>(*m_value);
        }

        template<typename T> const auto &get() const {
            return dynamic_cast<const T &>(*m_value);
        }

        template<typename T> auto *get_if() {
            return dynamic_cast<T *>(m_value.get());
        }

        template<typename T> const auto *get_if() const {
            return dynamic_cast<const T *>(m_value.get());
        }

        template<typename T> bool is() const {
            return get_if<T>() != nullptr;
        }

    private:
        std::shared_ptr<request_base> m_value;
    };
}

#endif