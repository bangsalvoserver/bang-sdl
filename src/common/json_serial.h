#ifndef __JSON_SERIAL_H__
#define __JSON_SERIAL_H__

#include "enum_variant.h"
#include "base64.h"

#include <json/json.h>
#include <vector>
#include <string>
#include <map>

namespace json {

    template<typename T>
    concept serializable = requires(T obj) {
        { obj.serialize() } -> std::convertible_to<Json::Value>;
    };

    template<typename T>
    struct serializer {
        Json::Value operator()(const T &value) const {
            return Json::Value(value);
        }
    };

    template<serializable T>
    struct serializer<T> {
        Json::Value operator()(const T &value) const {
            return value.serialize();
        }
    };

    template<enums::has_names T>
    struct serializer<T> {
        Json::Value operator()(const T &value) const {
            return std::string(enums::to_string(value));
        }
    };

    template<enums::has_names T> requires enums::flags_enum<T>
    struct serializer<T> {
        Json::Value operator()(const T &value) const {
            return enums::flags_to_string(value);
        }
    };

    template<typename T>
    struct serializer<std::vector<T>> {
        Json::Value operator()(const std::vector<T> &value) const {
            Json::Value ret = Json::arrayValue;
            for (const T &obj : value) {
                ret.append(serializer<T>{}(obj));
            }
            return ret;
        }
    };

    template<>
    struct serializer<std::vector<std::byte>> {
        Json::Value operator()(const std::vector<std::byte> &value) const {
            return base64_encode(value.data(), value.size());
        }
    };

    template<typename T>
    struct serializer<std::map<std::string, T>> {
        Json::Value operator()(const std::map<std::string, T> &value) const {
            Json::Value ret = Json::objectValue;
            for (const auto &[key, value] : value) {
                ret[key] = serializer<T>{}(value);
            }
            return ret;
        }
    };

    template<enums::reflected_enum T>
    struct serializer<enums::enum_variant<T>> {
        Json::Value operator()(const enums::enum_variant<T> &value) const {
            return enums::visit_indexed([]<T Value>(enums::enum_constant<Value>, auto && ... args) {
                Json::Value ret = Json::objectValue;
                ret["type"] = serializer<T>{}(Value);
                if constexpr (sizeof...(args) > 0) {
                    ret["value"] = serializer<enums::enum_type_t<Value>>{}(std::forward<decltype(args)>(args) ... );
                }
                return ret;
            }, value);
        }
    };

    template<typename T>
    Json::Value serialize(const T &value) {
        return serializer<T>{}(value);
    }

    template<typename T>
    concept deserializable = requires(Json::Value obj) {
        { T::deserialize(obj) } -> std::convertible_to<T>;
    };

    template<typename T>
    struct deserializer {
        T operator()(const Json::Value &value) const {
            return value.as<T>();
        }
    };

    template<deserializable T>
    struct deserializer<T> {
        T operator()(const Json::Value &value) const {
            return T::deserialize(value);
        }
    };

    template<enums::has_names T>
    struct deserializer<T> {
        T operator()(const Json::Value &value) const {
            return enums::from_string<T>(value.asString());
        }
    };

    template<enums::has_names T> requires enums::flags_enum<T>
    struct deserializer<T> {
        T operator()(const Json::Value &value) const {
            return enums::flags_from_string<T>(value.asString());
        }
    };

    template<typename T>
    struct deserializer<std::vector<T>> {
        std::vector<T> operator()(const Json::Value &value) const {
            std::vector<T> ret;
            for (const auto &obj : value) {
                ret.push_back(deserializer<T>{}(obj));
            }
            return ret;
        }
    };

    template<>
    struct deserializer<std::vector<std::byte>> {
        std::vector<std::byte> operator()(const Json::Value &value) const {
            return base64_decode(value.asString());
        }
    };

    template<typename T>
    struct deserializer<std::map<std::string, T>> {
        T operator()(const Json::Value &value) const {
            std::map<std::string, T> ret;
            for (auto it=value.begin(); it!=value.end(); ++it) {
                ret.emplace(it.key(), deserializer<T>{}(*it));
            }
            return ret;
        }
    };

    template<enums::reflected_enum T>
    struct deserializer<enums::enum_variant<T>> {
        enums::enum_variant<T> operator()(const Json::Value &value) const {
            return enums::visit_enum([&](auto enum_const) {
                constexpr T E = decltype(enum_const)::value;
                if constexpr (enums::has_type<E>) {
                    return enums::enum_variant<T>(enum_const, deserializer<enums::enum_type_t<E>>{}(value["value"]));
                } else {
                    return enums::enum_variant<T>(enum_const);
                }
            }, deserializer<T>{}(value["type"]));
        }
    };

    template<typename T>
    T deserialize(const Json::Value &value) {
        return deserializer<T>{}(value);
    }
}

#define DEFINE_STRUCT_FIELD(r, structName, tuple) \
    ENUM_TUPLE_TAIL(tuple) ENUM_ELEMENT_NAME(tuple){};

#define JSON_WRITE_FIELD(r, structName, tuple) \
    ret[BOOST_PP_STRINGIZE(ENUM_ELEMENT_NAME(tuple))] = json::serialize(ENUM_ELEMENT_NAME(tuple));

#define JSON_READ_FIELD(r, structName, tuple) \
    ret.ENUM_ELEMENT_NAME(tuple) = json::deserialize<ENUM_TUPLE_TAIL(tuple)>(value[BOOST_PP_STRINGIZE(ENUM_ELEMENT_NAME(tuple))]);

#define DO_DEFINE_SERIALIZABLE(structName, tupleSeq) \
    struct structName { \
        BOOST_PP_SEQ_FOR_EACH(DEFINE_STRUCT_FIELD, structName, tupleSeq) \
        Json::Value serialize() const { \
            Json::Value ret = Json::objectValue; \
            BOOST_PP_SEQ_FOR_EACH(JSON_WRITE_FIELD, structName, tupleSeq) \
            return ret; \
        } \
        static structName deserialize(const Json::Value &value) { \
            structName ret; \
            BOOST_PP_SEQ_FOR_EACH(JSON_READ_FIELD, structName, tupleSeq) \
            return ret; \
        } \
    };

#define DEFINE_SERIALIZABLE(structName, seq) DO_DEFINE_SERIALIZABLE(structName, ADD_PARENTHESES(seq))

#define DO_DEFINE_SERIALIZABLE_INHERITS(structName, baseName, tupleSeq) \
    struct structName : baseName { \
        static_assert(json::serializable<baseName>); \
        BOOST_PP_SEQ_FOR_EACH(DEFINE_STRUCT_FIELD, structName, tupleSeq) \
        Json::Value serialize() const { \
            Json::Value ret = baseName::serialize(); \
            BOOST_PP_SEQ_FOR_EACH(JSON_WRITE_FIELD, structName, tupleSeq) \
            return ret; \
        } \
        static structName deserialize(const Json::Value &value) { \
            structName ret; \
            static_cast<baseName &>(ret) = baseName::deserialize(value); \
            BOOST_PP_SEQ_FOR_EACH(JSON_READ_FIELD, structName, tupleSeq) \
            return ret; \
        } \
    };

#define DEFINE_SERIALIZABLE_INHERITS(structName, baseName, seq) DO_DEFINE_SERIALIZABLE_INHERITS(structName, baseName, ADD_PARENTHESES(seq))

#endif