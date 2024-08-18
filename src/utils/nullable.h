#ifndef __UTILS_NULLABLE_H__
#define __UTILS_NULLABLE_H__

#include "json_serial.h"

namespace utils {

    template<typename T> requires std::is_pointer_v<T>
    class nullable {
    public:
        using value_type = std::remove_pointer_t<T>;

        nullable() = default;
        nullable(T value) : value(value) {}

        operator T () { return value; }
        operator T () const { return value; }

        T get() { return value; }
        T get() const { return value; }

        value_type &operator *() { return *value; }
        value_type &operator *() const { return *value; }

        T operator -> () { return value; }
        T operator -> () const { return value; }

        explicit operator bool () const { return value != nullptr; }

    private:
        T value = nullptr;
    };

}

namespace json {

    template<typename T, typename Context> requires serializable<T, Context>
    struct serializer<utils::nullable<T>, Context> {
        json operator()(const utils::nullable<T> &value, const Context &ctx) const {
            if (value) {
                return serialize_unchecked(value.get(), ctx);
            } else {
                return json{};
            }
        }
    };

    template<typename T, typename Context> requires deserializable<T, Context>
    struct deserializer<utils::nullable<T>, Context> {
        utils::nullable<T> operator()(const json &value, const Context &ctx) const {
            if (value.is_null()) {
                return {};
            } else {
                return deserialize_unchecked<T>(value, ctx);
            }
        }
    };
}

#endif