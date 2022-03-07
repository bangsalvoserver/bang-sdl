#include "user_info.h"

#include "widgets/defaults.h"

#include <cstring>

constexpr uint8_t surface_magic_num = 0x8f;

namespace binary {

void serializer<sdl::surface>::operator()(const sdl::surface &image, byte_vector &out) const {
    serializer<uint8_t>{}(surface_magic_num, out);
    if (image) {
        const auto w = image.get_rect().w;
        const auto h = image.get_rect().h;
        const auto bpp = image.get()->format->BytesPerPixel;
        const auto nbytes = w * h * bpp;
        serializer<uint8_t>{}(w, out);
        serializer<uint8_t>{}(h, out);
        serializer<uint8_t>{}(bpp, out);
        out.resize(out.size() + nbytes);
        std::memcpy(out.data() + out.size() - nbytes, image.get()->pixels, nbytes);
    } else {
        serializer<uint8_t>{}(0, out);
        serializer<uint8_t>{}(0, out);
        serializer<uint8_t>{}(0, out);
    }
}

size_t serializer<sdl::surface>::get_size(const sdl::surface &image) const {
    if (image) {
        const auto w = image.get_rect().w;
        const auto h = image.get_rect().h;
        const auto bpp = image.get()->format->BytesPerPixel;
        return 4 + w * h * bpp;
    } else {
        return 4;
    }
}

sdl::surface deserializer<sdl::surface>::operator()(byte_ptr &pos, byte_ptr end) const {
    uint8_t m = deserializer<uint8_t>{}(pos, end);
    if (m != surface_magic_num) {
        throw binary::read_error(binary::read_error_code::magic_number_mismatch);
    }

    uint8_t w = deserializer<uint8_t>{}(pos, end);
    uint8_t h = deserializer<uint8_t>{}(pos, end);
    uint8_t bpp = deserializer<uint8_t>{}(pos, end);
    size_t nbytes = w * h * bpp;
    
    check_length(pos, end, nbytes);
    if (nbytes > 0) {
        SDL_Surface *surf = SDL_CreateRGBSurface(0, w, h, 8 * bpp, sdl::rmask, sdl::gmask, sdl::bmask, sdl::amask);
        SDL_LockSurface(surf);
        std::memcpy(surf->pixels, pos, nbytes);
        SDL_UnlockSurface(surf);

        pos += nbytes;
        return surf;
    }
    
    return {};
}

}