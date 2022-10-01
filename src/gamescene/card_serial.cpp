#include "game/card_serial.h"

#include "game.h"

namespace json {

Json::Value serializer<banggame::card_view *, banggame::game_scene>::operator()(banggame::card_view *card) const {
    if (card) {
        return card->id;
    } else {
        return Json::nullValue;
    }
}

Json::Value serializer<banggame::player_view *, banggame::game_scene>::operator()(banggame::player_view *player) const {
    if (player) {
        return player->id;
    } else {
        return Json::nullValue;
    }
}

Json::Value serializer<banggame::player_card_pair, banggame::game_scene>::operator()(banggame::player_card_pair pair) const {
    if (pair.first) {
        return pair.first->id;
    } else {
        return Json::nullValue;
    }
}

Json::Value serializer<banggame::card_cube_pair, banggame::game_scene>::operator()(banggame::card_cube_pair pair) const {
    if (pair.first) {
        return pair.first->id;
    } else {
        return Json::nullValue;
    }
}

banggame::card_view *deserializer<banggame::card_view *, banggame::game_scene>::operator()(const Json::Value &value) const {
    if (value.isNull()) {
        return nullptr;
    } else {
        return context.find_card(value.asInt());
    }
}

banggame::player_view *deserializer<banggame::player_view *, banggame::game_scene>::operator()(const Json::Value &value) const {
    if (value.isNull()) {
        return nullptr;
    } else {
        return context.find_player(value.asInt());
    }
}

}