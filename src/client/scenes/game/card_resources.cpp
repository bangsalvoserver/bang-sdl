#include "card_resources.h"

#include "utils/unpacker.h"

#include <SDL2/SDL.h>

namespace banggame {
    sdl::surface get_card_resource(std::string_view name) {
        static std::ifstream cards_pak_data(std::string(SDL_GetBasePath()) + "cards.pak", std::ios::in | std::ios::binary);
        static const unpacker card_resources(cards_pak_data);

        return sdl::surface(card_resources[name]);
    }
}