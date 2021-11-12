cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
pkg_check_modules(SDL2_net REQUIRED SDL2_net IMPORTED_TARGET)
pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf IMPORTED_TARGET)
pkg_check_modules(SDL2_image REQUIRED SDL2_image IMPORTED_TARGET)
pkg_check_modules(SDL2_gfx REQUIRED SDL2_gfx IMPORTED_TARGET)

find_package(Boost REQUIRED)

pkg_check_modules(jsoncpp REQUIRED jsoncpp IMPORTED_TARGET)

set(bangclient_srcs
    src/client/main.cpp
    src/client/manager.cpp
    src/client/scenes/game/card.cpp
    src/client/scenes/game/game.cpp
    src/client/scenes/game/game_ui.cpp
    src/client/scenes/game/target_finder.cpp
    src/client/scenes/widgets/button.cpp
    src/client/scenes/widgets/checkbox.cpp
    src/client/scenes/widgets/chat_ui.cpp
    src/client/scenes/widgets/textbox.cpp
    src/client/scenes/connect.cpp
    src/client/scenes/lobby_list.cpp
    src/client/scenes/lobby.cpp
    src/client/tinyfd/tinyfiledialogs.c
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
    src/server/game/player.cpp
    src/server/game/requests.cpp
    src/server/game/scenarios.cpp
)

include_directories(bangclient src)
add_subdirectory(resources)

add_executable(bangclient ${bangclient_srcs})
target_link_libraries(bangclient PkgConfig::SDL2 PkgConfig::SDL2_net PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx PkgConfig::jsoncpp)
target_link_libraries(bangclient resources_obj)
add_dependencies(bangclient cards_pak)
if(WIN32)
    target_link_options(bangclient PRIVATE -mconsole)
endif()

add_executable(bangserver ${bangserver_srcs})
target_link_libraries(bangserver PkgConfig::SDL2_net PkgConfig::jsoncpp)
target_link_libraries(bangserver bang_cards)
if(WIN32)
    target_link_options(bangserver PRIVATE -mconsole)
endif()