#ifndef __UTILS_REMOVE_DEFAULTS_H__
#define __UTILS_REMOVE_DEFAULTS_H__

#include <type_traits>

namespace utils {

    template<typename T> requires (std::is_aggregate_v<T> && std::is_default_constructible_v<T>)
    class remove_defaults {
    private:
        T value;
    
    public:
        remove_defaults() = default;

        remove_defaults(const T &value) : value(value) {}
        remove_defaults(T &&value) : value(std::move(value)) {}

        T &get() {
            return value;
        }

        const T &get() const {
            return value;
        }
    };

}

#endif