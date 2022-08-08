#include "image_serial.h"

#include <charconv>

namespace sdl {

static image_pixels do_surface_to_image_pixels(const surface &image) {
    image_pixels ret;
    SDL_LockSurface(image.get());
    ret.width = image.get()->w;
    ret.height = image.get()->h;
    const std::byte *pixels_ptr = static_cast<const std::byte *>(image.get()->pixels);
    ret.pixels.assign(pixels_ptr, pixels_ptr + image.get()->h * image.get()->pitch);
    SDL_UnlockSurface(image.get());
    return ret;
}
    
image_pixels surface_to_image_pixels(const surface &image) {
    if (!image) {
        return {};
    } else if (image.get()->format->BytesPerPixel == 4) {
        return do_surface_to_image_pixels(image);
    } else {
        return do_surface_to_image_pixels(SDL_ConvertSurfaceFormat(image.get(), SDL_PIXELFORMAT_RGBA8888, 0));
    }
}

surface image_pixels_to_surface(const image_pixels &image) {
    if (image.width == 0 || image.height == 0) {
        return {};
    }
    surface ret(image.width, image.height);
    SDL_LockSurface(ret.get());
    std::memcpy(ret.get()->pixels, image.pixels.data(), ret.get()->h * ret.get()->pitch);
    SDL_UnlockSurface(ret.get());
    return ret;
}

}

namespace binary {

constexpr uint8_t surface_magic_num = 0x8f;

void serializer<sdl::surface>::operator()(const sdl::surface &image, byte_vector &out) const {
    if (image) {
        serializer<uint8_t>{}(surface_magic_num, out);
        const auto w = image.get_rect().w;
        const auto h = image.get_rect().h;
        const auto bpp = image.get()->format->BytesPerPixel;
        const auto nbytes = w * h * bpp;
        serializer<uint8_t>{}(w, out);
        serializer<uint8_t>{}(h, out);
        serializer<uint8_t>{}(bpp, out);
        out.resize(out.size() + nbytes);
        std::memcpy(out.data() + out.size() - nbytes, image.get()->pixels, nbytes);
    }
}

size_t serializer<sdl::surface>::get_size(const sdl::surface &image) const {
    if (image) {
        const auto w = image.get_rect().w;
        const auto h = image.get_rect().h;
        const auto bpp = image.get()->format->BytesPerPixel;
        return 4 + w * h * bpp;
    } else {
        return 0;
    }
}

sdl::surface deserializer<sdl::surface>::operator()(byte_ptr &pos, byte_ptr end) const {
    if (pos == end) return {};

    uint8_t m = deserializer<uint8_t>{}(pos, end);
    if (m != surface_magic_num) {
        throw binary::read_error(binary::read_error_code::magic_number_mismatch);
    }

    uint8_t w = deserializer<uint8_t>{}(pos, end);
    uint8_t h = deserializer<uint8_t>{}(pos, end);
    uint8_t bpp = deserializer<uint8_t>{}(pos, end);
    size_t nbytes = w * h * bpp;
    
    check_length(pos, end, nbytes);

    SDL_Surface *surf = SDL_CreateRGBSurface(0, w, h, 8 * bpp, sdl::rmask, sdl::gmask, sdl::bmask, sdl::amask);
    SDL_LockSurface(surf);
    std::memcpy(surf->pixels, pos, nbytes);
    SDL_UnlockSurface(surf);

    pos += nbytes;
    return surf;
}

}

namespace json {

Json::Value serializer<sdl::color>::operator()(const sdl::color &value) const {
    Json::Value ret = Json::arrayValue;
    ret.append(value.r);
    ret.append(value.g);
    ret.append(value.g);
    ret.append(value.a);
    return ret;
}

sdl::color deserializer<sdl::color>::operator()(const Json::Value &value) const {
    if (value.isArray()) {
        return {
            uint8_t(value[0].asInt()),
            uint8_t(value[1].asInt()),
            uint8_t(value[2].asInt()),
            uint8_t(value.size() == 4 ? value[3].asInt() : 0xff)
        };
    } else if (value.isString()) {
        std::string str = value.asString();
        uint32_t ret;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), ret, 16);
        if (ec == std::errc{} && ptr == str.data() + str.size()) {
            if (value.size() == 6) {
                return sdl::rgb(ret);
            } else {
                return sdl::rgba(ret);
            }
        } else {
            return {};
        }
    } else if (value.isInt()) {
        return sdl::rgba(value.asInt());
    } else {
        return {};
    }
}

sdl::surface deserializer<sdl::surface>::operator()(const Json::Value &value) const {
    if (value.isString()) {
        return binary::deserialize<sdl::surface>(json::deserialize<std::vector<std::byte>>(value));
    } else if (!value.isNull()) {
        return sdl::image_pixels_to_surface(json::deserialize<sdl::image_pixels>(value));
    } else {
        return {};
    }
}

}