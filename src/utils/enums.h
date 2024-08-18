#ifndef __ENUMS_H__
#define __ENUMS_H__

#include <reflect>

#include "json_serial.h"
#include "static_map.h"

namespace enums {

    template<typename T> concept enumeral = std::is_enum_v<T>;

    template<enumeral T, typename ISeq> struct build_enum_values;
    template<enumeral T, size_t ... Is> struct build_enum_values<T, std::index_sequence<Is ...>> {
        static constexpr std::array value { static_cast<T>(reflect::enumerators<T>[Is].first) ... };
    };

    template<enumeral T> constexpr const auto &enum_values() {
        return build_enum_values<T, std::make_index_sequence<reflect::enumerators<T>.size()>>::value;
    }

    template<enumeral T>
    constexpr bool is_linear_enum() {
        size_t i=0;
        for (T value : enum_values<T>()) {
            if (static_cast<size_t>(value) != i) {
                return false;
            }
            ++i;
        }
        return true;
    }

    template<enumeral T>
    constexpr size_t indexof(T value) {
        constexpr const auto &values = enum_values<T>();
        if constexpr (is_linear_enum<T>()) {
            size_t result = static_cast<size_t>(value);
            if (result >= 0 && result <= static_cast<size_t>(values.back())) {
                return result;
            }
        } else {
            for (size_t i=0; i<values.size(); ++i) {
                if (values[i] == value) {
                    return i;
                }
            }
        }
        throw std::out_of_range("invalid enum index");
    }

    template<enumeral T>
    constexpr std::string_view to_string(T input) {
        return reflect::enumerators<T>[indexof(input)].second;
    }

    template<enumeral T>
    static constexpr auto names_to_values_map = []<size_t ... Is>(std::index_sequence<Is ...>) {
        constexpr auto &values = enum_values<T>();
        return utils::static_map<std::string_view, T>({
            { to_string(values[Is]), values[Is] } ...
        });
    }(std::make_index_sequence<enum_values<T>().size()>());

    template<enumeral T>
    constexpr std::optional<T> from_string(std::string_view str) {
        const auto &names_map = names_to_values_map<T>;
        if (auto it = names_map.find(str); it != names_map.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    template<enumeral auto ... Values> struct enum_sequence {
        static constexpr size_t size = sizeof...(Values);
    };
    
}

namespace json {

    template<enums::enumeral T, typename Context>
    struct deserializer<T, Context> {
        T operator()(const json &value) const {
            if (!value.is_string()) {
                throw deserialize_error(std::format("Cannot deserialize {}: value is not a string", reflect::type_name<T>()));
            }
            auto str = value.get<std::string>();
            if (auto ret = enums::from_string<T>(str)) {
                return *ret;
            } else {
                throw deserialize_error(std::format("Invalid {} value: {}", reflect::type_name<T>(), str));
            }
        }
    };

    template<enums::enumeral T, typename Context>
    struct serializer<T, Context> {
        json operator()(const T &value) const {
            return enums::to_string(value);
        }
    };

}

#endif