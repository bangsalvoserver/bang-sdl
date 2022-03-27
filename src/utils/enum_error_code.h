#ifndef __ENUM_ERROR_CODE_H__
#define __ENUM_ERROR_CODE_H__

#include <system_error>
#include "fmt/format.h"
#include "enums.h"

struct error_message {
    std::string_view message;
};

namespace enums {

    template<typename E> concept error_code_enum = full_data_enum_of_type<E, error_message>;

    template<error_code_enum E>
    struct error_code_enum_category : std::error_category {
        const char *name() const noexcept override {
            return enum_name_v<E>.data();
        }

        std::string message(int ev) const override {
            E value = index_to<E>(ev);
            if (is_valid_value(value)) {
                return std::string(get_data(value).message);
            } else {
                return fmt::format("Invalid {}", enum_name_v<E>);
            }
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
