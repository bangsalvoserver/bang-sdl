#ifndef __IMAGE_SERIAL_H__
#define __IMAGE_SERIAL_H__

#include "net/messages.h"
#include "utils/binary_serial.h"
#include "utils/json_serial.h"
#include "sdl_wrap.h"

namespace sdl {
    image_pixels surface_to_image_pixels(const surface &image);
    surface image_pixels_to_surface(const image_pixels &image);
}

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
        json operator()(const sdl::color &value) const;
    };

    template<> struct deserializer<sdl::color> {
        sdl::color operator()(const json &value) const;
    };

    template<> struct serializer<sdl::surface> {
        json operator()(const sdl::surface &image) const {
            return serialize(sdl::surface_to_image_pixels(image));
        }
    };

    template<> struct deserializer<sdl::surface> {
        sdl::surface operator()(const json &value) const;
    };
}

#endif