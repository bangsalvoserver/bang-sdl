#ifndef __BINARY_SERIAL_H__
#define __BINARY_SERIAL_H__

#include "enum_variant.h"
#include "reflector.h"

#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <map>

namespace binary {

    template<typename T> struct serializer {};

    using byte_vector = std::vector<std::byte>;

    constexpr size_t default_packet_size = 1024;

    template<typename T>
    concept serializable = requires(T value, byte_vector &out) {
        serializer<T>{}(value, out);
    };

    template<std::integral T>
    struct serializer<T> {
        void operator()(const T &value, byte_vector &out) const {
            [&]<size_t ... I>(std::index_sequence<I...>) {
                (out.push_back(static_cast<std::byte>((value & (static_cast<T>(0xff) << (8 * (sizeof(T) - I - 1)))) >> (8 * (sizeof(T) - I - 1)))), ...);
            }(std::make_index_sequence<sizeof(T)>());
        }
    };

    template<> struct serializer<std::string> {
        void operator()(const std::string &value, byte_vector &out) const {
            serializer<uint16_t>{}(value.size(), out);
            const auto *pos = reinterpret_cast<const std::byte *>(value.data());
            out.insert(out.end(), pos, pos + value.size());
        }
    };

    template<typename T> requires (std::is_enum_v<T>)
    struct serializer<T> {
        void operator()(const T &value, byte_vector &out) const {
            using underlying = std::underlying_type_t<T>;
            return serializer<underlying>{}(static_cast<underlying>(value), out);
        }
    };

    template<serializable T>
    struct serializer<std::vector<T>> {
        void operator()(const std::vector<T> &value, byte_vector &out) const {
            serializer<uint16_t>{}(value.size(), out);
            for (const T &obj : value) {
                serializer<T>{}(obj, out);
            }
        }
    };

    template<> struct serializer<byte_vector> {
        void operator()(const byte_vector &value, byte_vector &out) const {
            serializer<uint16_t>{}(value.size(), out);
            out.insert(out.end(), value.begin(), value.end());
        }
    };

    template<serializable T>
    struct serializer<std::map<std::string, T>> {
        void operator()(const std::map<std::string, T> &value, byte_vector &out) const {
            serializer<uint16_t>(value.size(), out);
            for (const auto &[key, value] : value) {
                serializer<std::string>{}(key, out);
                serializer<T>{}(value, out);
            }
        }
    };

    template<> struct serializer<std::monostate> {
        void operator()(const std::monostate &value, byte_vector &out) const {}
    };

    template<serializable ... Ts>
    struct serializer<std::variant<Ts ...>> {
        void operator()(const std::variant<Ts ...> &value, byte_vector &out) const {
            serializer<uint16_t>{}(value.index(), out);
            std::visit([&](const auto &value) {
                return serializer<std::remove_cvref_t<decltype(value)>>{}(value, out);
            }, value);
        }
    };

    template<enums::reflected_enum T>
    struct serializer<enums::enum_variant<T>> {
        void operator()(const enums::enum_variant<T> &value, byte_vector &out) const {
            using type = enums::enum_variant_base<T>;
            serializer<type>{}(static_cast<const type &>(value), out);
        }
    };

    template<reflector::reflectable T>
    struct serializer<T> {
        void operator()(const T &value, byte_vector &out) const {
            [&]<size_t ... I>(std::index_sequence<I ...>) {
                ([&]{
                    const auto field_data = reflector::get_field_data<I>(value);
                    const auto &field = field_data.get();
                    serializer<std::remove_cvref_t<decltype(field)>>{}(field, out);
                }(), ...);
            }(std::make_index_sequence<reflector::num_fields<T>>());
        }
    };

    template<serializable T>
    byte_vector serialize(const T &value) {
        byte_vector ret;
        ret.reserve(default_packet_size);
        serializer<T>{}(value, ret);
        return ret;
    }

    template<typename T> struct deserializer {};

    using byte_ptr = const std::byte *;

    template<typename T>
    concept deserializable = requires(byte_ptr &pos, byte_ptr end) {
        deserializer<T>{}(pos, end);
    };

    struct read_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    inline void check_length(byte_ptr pos, byte_ptr end, size_t len) {
        if (pos + len > end) {
            throw read_error("Buffer overflow");
        }
    }

    template<std::integral T>
    struct deserializer<T> {
        T operator()(byte_ptr &pos, byte_ptr end) const {
            check_length(pos, end, sizeof(T));
            T value = [&]<size_t ... I>(std::index_sequence<I ...>) {
                return ((static_cast<uint8_t>(*(pos + I)) << (8 * (sizeof(T) - I - 1))) | ...);
            }(std::make_index_sequence<sizeof(T)>());
            pos += sizeof(T);
            return value;
        }
    };

    template<> struct deserializer<std::string> {
        std::string operator()(byte_ptr &pos, byte_ptr end) const {
            auto size = deserializer<uint16_t>{}(pos, end);
            check_length(pos, end, size);
            std::string ret(size, '\0');
            std::memcpy(ret.data(), pos, size);
            pos += size;
            return ret;
        }
    };

    template<typename T> requires (std::is_enum_v<T>)
    struct deserializer<T> {
        T operator()(byte_ptr &pos, byte_ptr end) const {
            using underlying = std::underlying_type_t<T>;
            return static_cast<T>(deserializer<underlying>{}(pos, end));
        }
    };

    template<deserializable T>
    struct deserializer<std::vector<T>> {
        std::vector<T> operator()(byte_ptr &pos, byte_ptr end) const {
            auto size = deserializer<uint16_t>{}(pos, end);
            std::vector<T> ret;
            ret.reserve(size);
            for (uint16_t i=0; i<size; ++i) {
                ret.push_back(deserializer<T>{}(pos, end));
            }
            return ret;
        }
    };

    template<> struct deserializer<byte_vector> {
        byte_vector operator()(byte_ptr &pos, byte_ptr end) const {
            auto size = deserializer<uint16_t>{}(pos, end);
            check_length(pos, end, size);
            byte_vector ret;
            ret.reserve(size);
            ret.insert(ret.end(), pos, pos + size);
            pos += size;
            return ret;
        }
    };

    template<deserializable T>
    struct deserializer<std::map<std::string, T>> {
        std::map<std::string, T> operator()(byte_ptr &pos, byte_ptr end) const {
            auto size = deserializer<uint16_t>{}(pos, end);
            std::map<std::string, T> ret;
            for (uint16_t i=0; i<size; ++i) {
                std::string key = deserializer<std::string>{}(pos, end);
                T value = deserializer<T>{}(pos, end);
                ret.emplace(std::move(key), std::move(value));
            }
            return ret;
        }
    };

    template<> struct deserializer<std::monostate> {
        std::monostate operator()(byte_ptr &pos) const {
            return {};
        }
    };

    template<typename ... Ts>
    struct deserializer<std::variant<Ts...>> {
        using variant_type = std::variant<Ts...>;
        variant_type operator()(byte_ptr &pos, byte_ptr end) const {
            auto index = deserializer<uint16_t>{}(pos, end);
            constexpr auto lut = []<size_t ... I>(std::index_sequence<I...>){
                return std::array { +[](byte_ptr &pos, byte_ptr end) -> variant_type {
                    return deserializer<std::variant_alternative_t<I, variant_type>>{}(pos, end);
                } ... };
            }(std::make_index_sequence<sizeof...(Ts)>());
            return lut[index](pos, end);
        }
    };

    template<enums::reflected_enum T>
    struct deserializer<enums::enum_variant<T>> {
        enums::enum_variant<T> operator()(byte_ptr &pos, byte_ptr end) const {
            auto index = deserializer<uint16_t>{}(pos, end);
            constexpr auto lut = []<T ... Es>(enums::enum_sequence<Es ...>) {
                return std::array { +[](byte_ptr &pos, byte_ptr end) {
                    constexpr T enum_value = Es;
                    if constexpr (enums::has_type<enum_value>) {
                        return enums::enum_variant<T>{enums::enum_constant<enum_value>{}, deserializer<enums::enum_type_t<enum_value>>{}(pos, end)};
                    } else {
                        return enums::enum_variant<T>{enums::enum_constant<enum_value>{}};
                    }
                } ... };
            }(enums::make_enum_sequence<T>());
            return lut[index](pos, end);
        }
    };

    template<reflector::reflectable T> requires std::is_default_constructible_v<T>
    struct deserializer<T> {
        T operator()(byte_ptr &pos, byte_ptr end) const {
            T ret;
            [&]<size_t ... I>(std::index_sequence<I ...>) {
                ([&]{
                    auto &field = reflector::get_field_data<I>(ret).get();
                    field = deserializer<std::remove_cvref_t<decltype(field)>>{}(pos, end);
                }(), ...);
            }(std::make_index_sequence<reflector::num_fields<T>>());
            return ret;
        }
    };

    template<deserializable T>
    T deserialize(const byte_vector &data) {
        byte_ptr pos = data.data();
        byte_ptr end = pos + data.size();
        T obj = deserializer<T>{}(pos, end);
        if (pos != end) {
            throw read_error("Buffer underflow");
        }
        return obj;
    }
}

#endif