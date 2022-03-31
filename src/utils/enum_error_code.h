#ifndef __ENUM_ERROR_CODE_H__
#define __ENUM_ERROR_CODE_H__

#include <system_error>
#include "fmt/format.h"
#include "enums.h"

namespace enums {

    struct error_message {
        std::string_view message;
    };

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

#define CREATE_ENUM_ERROR_CODE_VALUE(r, _, elementTuple) (ENUM_ELEMENT_NAME(elementTuple), enums::error_message{ ENUM_TUPLE_TAIL(elementTuple) })

#define DEFINE_ENUM_ERROR_CODE_IMPL(enumName, enumTupleSeq) \
    DEFINE_ENUM_DATA(enumName, BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_ERROR_CODE_VALUE, _, enumTupleSeq)) \
    using enums::make_error_code;

#define DEFINE_ENUM_ERROR_CODE(enumName, enumElements) DEFINE_ENUM_ERROR_CODE_IMPL(enumName, ADD_PARENTHESES(enumElements))

#endif
