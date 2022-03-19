#ifndef __HOLDERS_H__
#define __HOLDERS_H__

#include <concepts>
#include <memory>

#include "utils/reflector.h"
#include "effects/card_effect.h"
#include "game_action.h"

namespace banggame {

    void handle_multitarget(card *origin_card, player *origin, mth_target_list targets);

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
        using effect_base<effect_type>::effect_base;

        bool can_respond(card *origin_card, player *target) const;

        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin, effect_flags flags);
        
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags);
        
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags);
    };
    
    struct equip_holder : effect_base<equip_type> {
        using effect_base<equip_type>::effect_base;

        void on_pre_equip(card *target_card, player *target);
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
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

        bool can_pick(card_pile_type pile, player *target, card *target_card) const {
            return m_value->can_pick(pile, target, target_card);
        }
        void on_pick(card_pile_type pile, player *target, card *target_card) {
            auto copy = m_value;
            copy->on_pick(pile, target, target_card);
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