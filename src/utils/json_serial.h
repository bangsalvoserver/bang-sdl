#ifndef __JSON_SERIAL_H__
#define __JSON_SERIAL_H__

#include <nlohmann/json.hpp>

#include <ranges>
#include <vector>
#include <chrono>

namespace json {

    using json = nlohmann::ordered_json;

    template<typename T>
    concept is_complete = requires(T self) { sizeof(self); };

    template<typename T, typename Context> struct serializer;

    template<typename T, typename Context>
    concept serializable = is_complete<serializer<T, Context>>;

    template<typename T, typename Context> struct deserializer;

    template<typename T, typename Context>
    concept deserializable = is_complete<deserializer<T, Context>>;

    using json_error = json::exception;

    struct serialize_error : json_error {
        serialize_error(const char *message): json_error(0, message) {}
        serialize_error(const std::string &message): json_error(0, message.c_str()) {}
    };

    struct deserialize_error : json_error {
        deserialize_error(const char *message): json_error(0, message) {}
        deserialize_error(const std::string &message): json_error(0, message.c_str()) {}
    };

    struct no_context {};

    template<typename T, typename Context> requires serializable<T, Context>
    json serialize_unchecked(const T &value, const Context &context) {
        serializer<T, Context> obj{};
        if constexpr (requires { obj(value, context); }) {
            return obj(value, context);
        } else {
            return obj(value);
        }
    }

    template<typename T, typename Context> requires serializable<T, Context>
    json serialize(const T &value, const Context &context) {
        try {
            return serialize_unchecked(value, context);
        } catch (const serialize_error &) {
            throw;
        } catch (const std::exception &e) {
            throw serialize_error(e.what());
        }
    }

    template<typename T> requires serializable<T, no_context>
    json serialize(const T &value) {
        return serialize(value, no_context{});
    }

    template<typename T, typename Context> requires deserializable<T, Context>
    auto deserialize_unchecked(const json &value, const Context &context) {
        deserializer<T, Context> obj{};
        if constexpr (requires { obj(value, context); }) {
            return obj(value, context);
        } else {
            return obj(value);
        }
    }

    template<typename T, typename Context> requires deserializable<T, Context>
    T deserialize(const json &value, const Context &context) {
        try {
            return deserialize_unchecked<T>(value, context);
        } catch (const deserialize_error &) {
            throw;
        } catch (const std::exception &e) {
            throw deserialize_error(e.what());
        }
    }

    template<typename T> requires deserializable<T, no_context>
    T deserialize(const json &value) {
        return deserialize<T>(value, no_context{});
    }

    template<typename T, typename Context> requires (
        !std::is_enum_v<T> && std::convertible_to<T, json>
    )
    struct serializer<T, Context> {
        json operator()(const T &value) const {
            return value;
        }
    };

    template<std::ranges::range Range, typename Context> requires (
        !std::convertible_to<Range, json>
        && serializable<std::ranges::range_value_t<Range>, Context>
    )
    struct serializer<Range, Context> {
        json operator()(const Range &value, const Context &ctx) const {
            auto ret = json::array();
            if constexpr (std::ranges::sized_range<Range>) {
                ret.get_ptr<json::array_t*>()->reserve(value.size());
            }
            for (const auto &obj : value) {
                ret.push_back(serialize_unchecked(obj, ctx));
            }
            return ret;
        }
    };

    template<typename Rep, typename Period, typename Context>
    struct serializer<std::chrono::duration<Rep, Period>, Context> {
        json operator()(const std::chrono::duration<Rep, Period> &value) const {
            return value.count();
        }
    };

    template<typename T, typename Context> requires serializable<T, Context>
    struct serializer<std::optional<T>, Context> {
        json operator()(const std::optional<T> &value, const Context &ctx) const {
            if (value) {
                return serialize_unchecked(*value, ctx);
            } else {
                return json{};
            }
        }
    };

    template<typename First, typename Second, typename Context>
    requires (serializable<First, Context> && serializable<Second, Context>)
    struct serializer<std::pair<First, Second>, Context> {
        json operator()(const std::pair<First, Second> &value, const Context &ctx) const {
            return json::array({
                serialize_unchecked(value.first, ctx),
                serialize_unchecked(value.second, ctx)
            });
        }
    };
    
    template<typename Context>
    struct deserializer<json, Context> {
        json operator()(const json &value) const {
            return value;
        }
    };

    template<typename T, typename Context> requires std::is_arithmetic_v<T>
    struct deserializer<T, Context> {
        T operator()(const json &value) const {
            if constexpr (std::is_same_v<T, bool>) {
                if (!value.is_boolean()) {
                    throw deserialize_error("Cannot deserialize boolean");
                }
            } else if constexpr (std::is_integral_v<T>) {
                if (!value.is_number_integer()) {
                    throw deserialize_error("Cannot deserialize integer");
                }
            } else {
                if (!value.is_number()) {
                    throw deserialize_error("Cannot deserialize number");
                }
            }
            return value.get<T>();
        }
    };

    template<typename Context>
    struct deserializer<std::string, Context> {
        std::string operator()(const json &value) const {
            if (!value.is_string()) {
                throw deserialize_error("Cannot deserialize string");
            }
            return value.get<std::string>();
        }
    };
    
    template<typename T, typename Context> requires deserializable<T, Context>
    struct deserializer<std::vector<T>, Context> {
        std::vector<T> operator()(const json &value, const Context &ctx) const {
            if (!value.is_array()) {
                throw deserialize_error("Cannot deserialize vector");
            }
            std::vector<T> ret;
            ret.reserve(value.size());
            for (const auto &obj : value) {
                ret.push_back(deserialize_unchecked<T>(obj, ctx));
            }
            return ret;
        }
    };

    template<typename Rep, typename Period, typename Context>
    struct deserializer<std::chrono::duration<Rep, Period>, Context> {
        std::chrono::duration<Rep, Period> operator()(const json &value) const {
            if (!value.is_number()) {
                throw deserialize_error("Cannot deserialize duration: value is not a number");
            }
            return std::chrono::duration<Rep, Period>{value.get<Rep>()};
        }
    };

    template<typename T, typename Context> requires deserializable<T, Context>
    struct deserializer<std::optional<T>, Context> {
        std::optional<T> operator()(const json &value, const Context &ctx) const {
            if (value.is_null()) {
                return std::nullopt;
            } else {
                return deserialize_unchecked<T>(value, ctx);
            }
        }
    };

    template<typename First, typename Second, typename Context>
    requires (deserializable<First, Context> && deserializable<Second, Context>)
    struct deserializer<std::pair<First, Second>, Context> {
        std::pair<First, Second> operator()(const json &value, const Context &ctx) const {
            if (!value.is_array() || value.size() != 2) {
                throw deserialize_error("Cannot deserialize pair: value is not an array of two elements");
            }
            return {
                deserialize_unchecked<First>(value[0], ctx),
                deserialize_unchecked<Second>(value[1], ctx)
            };
        }
    };
}

#endif