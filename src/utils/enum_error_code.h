#ifndef __ENUM_ERROR_CODE_H__
#define __ENUM_ERROR_CODE_H__

#include <system_error>
#include "enums.h"

struct error_message {
    std::string_view message;
};

namespace enums {
    namespace detail {
        template<reflected_enum auto ... Es> requires (value_with_data<Es> && ...)
        constexpr bool all_error_messages(enum_sequence<Es ...>) {
            return (std::convertible_to<enum_data_t<Es>, error_message> && ...);
        }
    }

    template<typename E>
    concept error_code_enum = requires {
        requires reflected_enum<E>;
        requires detail::all_error_messages(make_enum_sequence<E>());
    };

    template<error_code_enum E>
    struct error_code_enum_category : std::error_category {
        const char *name() const noexcept override {
            return enum_name_v<E>.data();
        }

        std::string message(int ev) const override {
            return std::string(get_data(static_cast<E>(ev)).message);
        }

        static inline const error_code_enum_category instance;
    };

    template<error_code_enum E>
    inline std::error_code make_error_code(E ec) {
        return {static_cast<int>(ec), error_code_enum_category<E>::instance};
    }
}

namespace std {
    template<enums::error_code_enum E> struct is_error_code_enum<E> : std::true_type {};
}

#endif
