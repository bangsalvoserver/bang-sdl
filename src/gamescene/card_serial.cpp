#include "game/card_serial.h"

#include "game.h"

namespace json {

template<> Json::Value serializer<banggame::card_view *, banggame::game_scene>::operator()(banggame::card_view *card) const {
    if (card) {
        return card->id;
    } else {
        return Json::nullValue;
    }
}

template<> Json::Value serializer<banggame::player_view *, banggame::game_scene>::operator()(banggame::player_view *player) const {
    if (player) {
        return player->id;
    } else {
        return Json::nullValue;
    }
}

template<> Json::Value serializer<banggame::player_card_pair, banggame::game_scene>::operator()(banggame::player_card_pair pair) const {
    if (pair.second) {
        return pair.second->id;
    } else {
        return Json::nullValue;
    }
}

template<> Json::Value serializer<banggame::card_cube_pair, banggame::game_scene>::operator()(banggame::card_cube_pair pair) const {
    if (pair.first) {
        return pair.first->id;
    } else {
        return Json::nullValue;
    }
}

template<> banggame::card_view *deserializer<banggame::card_view *, banggame::game_scene>::operator()(const Json::Value &value) const {
    if (value.isNull()) {
        return nullptr;
    } else {
        return context.find_card(value.asInt());
    }
}

template<> banggame::player_view *deserializer<banggame::player_view *, banggame::game_scene>::operator()(const Json::Value &value) const {
    if (value.isNull()) {
        return nullptr;
    } else {
        return context.find_player(value.asInt());
    }
}

}