#ifndef __USER_INFO_H__
#define __USER_INFO_H__

#include <vector>
#include <string>

#include "utils/sdl.h"
#include "utils/binary_serial.h"

namespace binary {

    template<> struct serializer<sdl::surface> {
        void operator()(const sdl::surface &image, byte_vector &out) const;
        size_t get_size(const sdl::surface &image) const;
    };

    template<> struct deserializer<sdl::surface> {
        sdl::surface operator()(byte_ptr &pos, byte_ptr end) const;
    };
}

struct user_info {
    std::string name;
    sdl::texture profile_image;

    user_info(std::string name, sdl::surface &&data)
        : name(std::move(name))
        , profile_image(std::move(data)) {}
};

#endif