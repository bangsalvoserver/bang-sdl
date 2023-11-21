#ifndef __IMAGE_SERIAL_H__
#define __IMAGE_SERIAL_H__

#include "net/messages.h"
#include "utils/json_serial.h"
#include "sdl_wrap.h"

namespace sdl {

    image_pixels surface_to_image_pixels(const surface &image);

    surface image_pixels_to_surface(const image_pixels &image);

}

namespace json {

    template<> struct serializer<sdl::surface> {
        json operator()(const sdl::surface &image) const {
            if (image) {
                return serialize(sdl::surface_to_image_pixels(image));
            } else {
                return json{};
            }
        }
    };

    template<> struct deserializer<sdl::surface> {
        sdl::surface operator()(const json &value) const {
            if (!value.is_null()) {
                return sdl::image_pixels_to_surface(deserialize<sdl::image_pixels>(value));
            } else {
                return {};
            }
        }
    };
}

#endif