#ifndef __USER_INFO_H__
#define __USER_INFO_H__

#include <vector>
#include <string>

#include "common/sdl.h"

sdl::surface decode_profile_image(const std::vector<std::byte> &data);
std::vector<std::byte> encode_profile_image(sdl::surface image);

struct user_info {
    std::string name;
    sdl::texture profile_image;

    user_info(std::string name, const std::vector<std::byte> &data)
        : name(std::move(name))
        , profile_image(decode_profile_image(data)) {}
};

#endif