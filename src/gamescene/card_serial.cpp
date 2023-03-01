#include "cards/card_serial.h"

#include "game.h"

namespace json {

template<> json serializer<banggame::card_view *, banggame::game_context_view>::operator()(banggame::card_view *card) const {
    if (card) {
        return card->id;
    } else {
        return {};
    }
}

template<> json serializer<banggame::player_view *, banggame::game_context_view>::operator()(banggame::player_view *player) const {
    if (player) {
        return player->id;
    } else {
        return {};
    }
}

template<> banggame::card_view *deserializer<banggame::card_view *, banggame::game_context_view>::operator()(const json &value) const {
    if (value.is_number_integer()) {
        return context.find_card(value.get<int>());
    } else {
        return nullptr;
    }
}

template<> banggame::player_view *deserializer<banggame::player_view *, banggame::game_context_view>::operator()(const json &value) const {
    if (value.is_number_integer()) {
        return context.find_player(value.get<int>());
    } else {
        return nullptr;
    }
}

}