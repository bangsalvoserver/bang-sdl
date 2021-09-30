#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include <concepts>

#include "card_enums.h"

namespace banggame {

    struct player;

    struct card_effect {
        virtual ~card_effect() {}

        target_type target = target_type::none;
        int maxdistance = 0;

        virtual void on_equip(player *target, int card_id) { }
        virtual void on_unequip(player *target, int card_id) { }

        virtual bool can_play(player *target) const { return true; }
        virtual void on_play(player *origin) { }
        virtual void on_play(player *origin, player *target) { }
        virtual void on_play(player *origin, player *target, int card_id) { }

        virtual void on_predraw_check(player *target, int card_id) { }
    };

    template<typename T>
    class vbase_holder {
    private:
        template<std::derived_from<T> U> struct type_tag{};

        std::byte data[sizeof(T)];

        template<std::derived_from<T> U>
        vbase_holder(type_tag<U>) noexcept {
            new (data) U;
        }
    
    public:
        vbase_holder() noexcept {
            new (data) T;
        }

        ~vbase_holder() noexcept {
            get()->~T();
        }

        vbase_holder(const vbase_holder &other) = default;
        vbase_holder(vbase_holder &&other) = default;
        
        vbase_holder &operator = (const vbase_holder &other) = default;
        vbase_holder &operator = (vbase_holder &&other) = default;

        T *get() noexcept { return reinterpret_cast<T *>(data); }
        const T *get() const noexcept { return reinterpret_cast<const T *>(data); }

        T &operator *() noexcept { return *get(); }
        const T &operator *() const noexcept { return *get(); }

        T *operator ->() noexcept { return get(); }
        const T *operator ->() const noexcept { return get(); }

        template<std::derived_from<T> U> static vbase_holder make() {
            static_assert(sizeof(U) == sizeof(T));
            return vbase_holder(type_tag<U>{});
        }

        template<std::derived_from<T> U> const U *as() const {
            return dynamic_cast<const U *>(get());
        }

        template<std::derived_from<T> U> U *as() {
            return dynamic_cast<U *>(get());
        }

        template<std::derived_from<T> U> bool is() const {
            return as<U>() != nullptr;
        }
    };

    using effect_holder = vbase_holder<card_effect>;
}

#endif