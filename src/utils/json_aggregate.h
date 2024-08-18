#ifndef __JSON_AGGREGATE_H__
#define __JSON_AGGREGATE_H__

#include "json_serial.h"
#include "remove_defaults.h"

#include <reflect>

namespace json {

    template<typename T>
    concept aggregate = std::is_aggregate_v<T>;

    template<typename T, typename Context>
    concept all_fields_serializable = aggregate<T> && 
        []<size_t ... Is>(std::index_sequence<Is ...>) {
            return (serializable<reflect::member_type<Is, T>, Context> && ...);
        }(std::make_index_sequence<reflect::size<T>()>());

    template<typename T, typename Context>
    concept all_fields_deserializable = aggregate<T> &&
        []<size_t ... Is>(std::index_sequence<Is ...>) {
            return (deserializable<reflect::member_type<Is, T>, Context> && ...);
        }(std::make_index_sequence<reflect::size<T>()>());

    template<aggregate T, typename Context>
    struct aggregate_serializer_unchecked {
        json operator()(const T &value, const Context &ctx) const {
            return [&]<size_t ... Is>(std::index_sequence<Is ...>) {
                return json::object({
                    {
                        reflect::member_name<Is, T>(),
                        serialize_unchecked(reflect::get<Is>(value), ctx)
                    } ... 
                });
            }(std::make_index_sequence<reflect::size<T>()>());
        }
    };

    template<aggregate T, typename Context> requires all_fields_serializable<T, Context>
    struct serializer<T, Context> : aggregate_serializer_unchecked<T, Context> {};

    template<aggregate T, typename Context>
    struct aggregate_deserializer_unchecked {
        template<size_t I>
        reflect::member_type<I, T> deserialize_field(const json &value, const Context &ctx) const {
            static constexpr auto name = reflect::member_name<I, T>();
            using value_type = reflect::member_type<I, T>;
            if (auto it = value.find(name); it != value.end()) {
                return deserialize_unchecked<value_type>(*it, ctx);
            } else {
                throw deserialize_error(std::format("Cannot deserialize {}: missing field {}", reflect::type_name<T>(), name));
            }
        }

        T operator()(const json &value, const Context &ctx) const {
            if (!value.is_object()) {
                throw deserialize_error(std::format("Cannot deserialize {}: value is not an object", reflect::type_name<T>()));
            }
            return [&]<size_t ... Is>(std::index_sequence<Is ...>) {
                return T{ deserialize_field<Is>(value, ctx) ... };
            }(std::make_index_sequence<reflect::size<T>()>());
        }
    };
    
    template<aggregate T, typename Context> requires all_fields_deserializable<T, Context>
    struct deserializer<T, Context> : aggregate_deserializer_unchecked<T, Context> {};

    template<typename T, typename Context> requires all_fields_serializable<T, Context>
    struct serializer<utils::remove_defaults<T>, Context>  {
        json operator()(const utils::remove_defaults<T> &value, const Context &ctx) const {
            json result;
            reflect::for_each<T>([&](auto I) {
                const auto &member_value = reflect::get<I>(value.get());
                using member_type = reflect::member_type<I, T>;
                if (member_value != member_type{}) {
                    if (result.is_null()) {
                        result = json::object();
                    }
                    result.push_back({
                        reflect::member_name<I, T>(),
                        serialize_unchecked(member_value, ctx)
                    });
                }
            });
            return result;
        }
    };

    template<typename T, typename Context> requires all_fields_deserializable<T, Context>
    struct deserializer<utils::remove_defaults<T>, Context> {
        using value_type = utils::remove_defaults<T>;

        value_type operator()(const json &value, const Context &ctx) const {
            if (value.is_null()) {
                return value_type{};
            } else {
                return value_type{aggregate_deserializer_unchecked<T, Context>{*this}(value)};
            }
        }
    };

}

#endif