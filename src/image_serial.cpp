#include "image_serial.h"

#include <charconv>

namespace sdl {

    static image_pixels do_surface_to_image_pixels(const surface &image) {
        image_pixels ret;
        SDL_LockSurface(image.get());
        ret.width = image.get()->w;
        ret.height = image.get()->h;
        const std::byte *pixels_ptr = static_cast<const std::byte *>(image.get()->pixels);
        ret.pixels.bytes.assign(pixels_ptr, pixels_ptr + image.get()->h * image.get()->pitch);
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
        std::memcpy(ret.get()->pixels, image.pixels.bytes.data(), ret.get()->h * ret.get()->pitch);
        SDL_UnlockSurface(ret.get());
        return ret;
    }

}