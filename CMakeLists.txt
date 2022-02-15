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
    src/client/intl.cpp
    src/client/entrypoint.cpp
    src/client/manager.cpp
    src/client/user_info.cpp
    src/client/chat_ui.cpp
    src/client/game/card.cpp
    src/client/game/player.cpp
    src/client/game/game.cpp
    src/client/game/game_ui.cpp
    src/client/game/target_finder.cpp
    src/client/scenes/connect.cpp
    src/client/scenes/lobby_list.cpp
    src/client/scenes/lobby.cpp
    src/client/widgets/button.cpp
    src/client/widgets/checkbox.cpp
    src/client/widgets/textbox.cpp
    src/client/widgets/text_list.cpp
    src/client/widgets/os_api.cpp
    src/client/tinyfd/tinyfiledialogs.c
)

set(bangserver_srcs
    src/server/server.cpp
    src/server/manager.cpp
    src/server/game/effects/characters_armedanddangerous.cpp
    src/server/game/effects/characters_base.cpp
    src/server/game/effects/characters_dodgecity.cpp
    src/server/game/effects/characters_goldrush.cpp
    src/server/game/effects/characters_valleyofshadows.cpp
    src/server/game/effects/characters_wildwestshow.cpp
    src/server/game/effects/effects_armedanddangerous.cpp
    src/server/game/effects/effects_base.cpp
    src/server/game/effects/effects_canyondiablo.cpp
    src/server/game/effects/effects_goldrush.cpp
    src/server/game/effects/effects_valleyofshadows.cpp
    src/server/game/effects/equips_armedanddangerous.cpp
    src/server/game/effects/equips_base.cpp
    src/server/game/effects/equips_canyondiablo.cpp
    src/server/game/effects/equips_goldrush.cpp
    src/server/game/effects/equips_valleyofshadows.cpp
    src/server/game/effects/requests_armedanddangerous.cpp
    src/server/game/effects/requests_base.cpp
    src/server/game/effects/requests_canyondiablo.cpp
    src/server/game/effects/requests_valleyofshadows.cpp
    src/server/game/effects/scenarios_highnoon.cpp
    src/server/game/effects/scenarios_fistfulofcards.cpp
    src/server/game/holders.cpp
    src/server/game/events.cpp
    src/server/game/game.cpp
    src/server/game/make_all_cards.cpp
    src/server/game/player.cpp
    src/server/game/possible_to_play.cpp
)

include_directories(src)
add_subdirectory(resources)

add_library(bangclient SHARED ${bangclient_srcs})
target_compile_definitions(bangclient PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(bangclient PkgConfig::SDL2 PkgConfig::SDL2_net PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx PkgConfig::jsoncpp)
target_link_libraries(bangclient resources_obj fonts_obj locale_headers)

find_package(fmt REQUIRED)
if (TARGET fmt::fmt-header-only)
    target_link_libraries(bangclient fmt::fmt-header-only)
else()
    target_link_libraries(bangclient fmt::fmt)
endif()

add_dependencies(bangclient cards_pak)

add_library(banggame SHARED ${bangserver_srcs})
target_link_libraries(banggame PkgConfig::SDL2_net PkgConfig::jsoncpp)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(banggame bang_cards)
    target_compile_definitions(banggame PRIVATE BANG_CARDS_JSON_LINKED)
    target_compile_definitions(banggame PRIVATE DISABLE_TESTING)
endif()
target_link_libraries(bangclient banggame)

add_executable(bangserver src/server/main.cpp)
target_link_libraries(bangserver banggame)
if(WIN32 AND MINGW)
    target_link_options(bangserver PRIVATE -mconsole)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (WIN32)
        add_executable(banglauncher WIN32 src/client/main.c resources/icons/bang.rc)
        if (MINGW)
            target_link_options(banglauncher PRIVATE -mconsole)
        endif()
    else()
        add_executable(banglauncher src/client/main.c)
    endif()

    target_link_libraries(banglauncher bangclient)
endif()