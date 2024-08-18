#ifndef __ENUM_BITSET_H__
#define __ENUM_BITSET_H__

#include "enums.h"
#include "json_serial.h"

#include <cstdint>
#include <type_traits>
#include <initializer_list>

namespace enums {
    using bitset_int = uint64_t;

    template<enumeral T>
    class bitset {
    private:
        bitset_int m_value = 0;
    
    public:
        constexpr bitset() = default;

        constexpr bitset(T value) {
            add(value);
        }

        constexpr bitset(std::initializer_list<T> values) {
            for (T value : values) {
                add(value);
            }
        }

    public:
        static constexpr bitset_int to_bit(T value) {
            return static_cast<bitset_int>(1) << indexof(value);
        }

        constexpr void merge(bitset value) {
            m_value |= value.m_value;
        }

        constexpr void add(T value) {
            m_value |= to_bit(value);
        }

        constexpr void remove(T value) {
            m_value &= ~to_bit(value);
        }

        constexpr void clear() {
            m_value = 0;
        }

        constexpr bool empty() const {
            return m_value == 0;
        }

        constexpr bool check(T value) const {
            return (m_value & to_bit(value)) != 0;
        }

        constexpr bool check(bitset value) const {
            return (m_value & value.m_value) == m_value;
        }
    };

}

namespace json {

    template<enums::enumeral T, typename Context>
    struct serializer<enums::bitset<T>, Context> {
        json operator()(const enums::bitset<T> &value) const {
            auto ret = json::array();
            for (T v : enums::enum_values<T>()) {
                if (value.check(v)) {
                    ret.push_back(enums::to_string(v));
                }
            }
            return ret;
        }
    };

    template<enums::enumeral T, typename Context>
    struct deserializer<enums::bitset<T>, Context> {
        enums::bitset<T> operator()(const json &value, const Context &ctx) const {
            if (!value.is_array()) {
                throw deserialize_error(std::format("Cannot deserialize {} bitset: value is not an array", reflect::type_name<T>()));
            }
            enums::bitset<T> ret;
            for (const auto &elem : value) {
                ret.add(deserialize_unchecked<T>(elem, ctx));
            }
            return ret;
        }
    };

}

#endif