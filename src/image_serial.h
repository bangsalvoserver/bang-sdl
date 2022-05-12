#ifndef __IMAGE_SERIAL_H__
#define __IMAGE_SERIAL_H__

#include "game/messages.h"
#include "utils/binary_serial.h"
#include "utils/json_serial.h"
#include "utils/sdl.h"

namespace binary {

    template<> struct serializer<sdl::surface> {
        void operator()(const sdl::surface &image, byte_vector &out) const;
        size_t get_size(const sdl::surface &image) const;
    };

    template<> struct deserializer<sdl::surface> {
        sdl::surface operator()(byte_ptr &pos, byte_ptr end) const;
    };
}

namespace json {
    
    template<> struct serializer<sdl::color> {
        Json::Value operator()(const sdl::color &value) const;
    };

    template<> struct deserializer<sdl::color> {
        sdl::color operator()(const Json::Value &value) const;
    };

    template<> struct serializer<sdl::surface> {
        Json::Value operator()(const sdl::surface &image) const {
            return json::serialize(sdl::image_pixels(image));
        }
    };

    template<> struct deserializer<sdl::surface> {
        sdl::surface operator()(const Json::Value &value) const;
    };

    template<> struct serializer<sdl::texture> {
        Json::Value operator()(const sdl::texture &value) const {
            return json::serialize(value.get_surface());
        }
    };

    template<> struct deserializer<sdl::texture> {
        sdl::texture operator()(const Json::Value &value) const {
            return json::deserialize<sdl::surface>(value);
        }
    };
}

#endif