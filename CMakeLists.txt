cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED SDL2 IMPORTED_TARGET)
pkg_check_modules(SDL2_net REQUIRED SDL2_net IMPORTED_TARGET)
pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf IMPORTED_TARGET)
pkg_check_modules(SDL2_image REQUIRED SDL2_image IMPORTED_TARGET)

find_package(jsoncpp REQUIRED)

set(bangclient_srcs
    src/client/main.cpp
    src/client/manager.cpp
    src/client/scenes/game/card.cpp
    src/client/scenes/game/game.cpp
    src/client/scenes/game/game_ui.cpp
    src/client/scenes/widgets/button.cpp
    src/client/scenes/widgets/chat_ui.cpp
    src/client/scenes/widgets/textbox.cpp
    src/client/scenes/connect.cpp
    src/client/scenes/lobby_list.cpp
    src/client/scenes/lobby.cpp
    src/client/scenes/make_lobby.cpp
)

set(bangserver_srcs
    src/server/main.cpp
    src/server/manager.cpp
    src/server/game/effect_holder.cpp
    src/server/game/effects.cpp
    src/server/game/equips.cpp
    src/server/game/characters.cpp
    src/server/game/game.cpp
    src/server/game/make_all_cards.cpp
    src/server/game/make_equip.cpp
    src/server/game/player.cpp
    src/server/game/requests.cpp
    src/server/game/timer.cpp
)

include_directories(bangclient src)
add_subdirectory(resources)

find_package(Boost REQUIRED)

add_executable(bangclient ${bangclient_srcs})
target_link_libraries(bangclient PkgConfig::SDL2 PkgConfig::SDL2_net PkgConfig::SDL2_ttf PkgConfig::SDL2_image jsoncpp_static)
target_link_options(bangclient PRIVATE -mconsole)

option(no_link_resources "Non includere le risorse nell'eseguibile" OFF)
if(no_link_resources)
    target_compile_definitions(bangclient PRIVATE NO_LINK_RESOURCES)
else()
    target_link_libraries(bangclient fonts card_images)
endif()

add_executable(bangserver ${bangserver_srcs})
target_link_libraries(bangserver PkgConfig::SDL2_net jsoncpp_static bang_cards)
target_link_options(bangserver PRIVATE -mconsole)