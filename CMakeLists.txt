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
    src/client/main.cpp
    src/client/manager.cpp
    src/client/user_info.cpp
    src/client/scenes/game/card_resources.cpp
    src/client/scenes/game/card.cpp
    src/client/scenes/game/player.cpp
    src/client/scenes/game/game.cpp
    src/client/scenes/game/game_ui.cpp
    src/client/scenes/game/target_finder.cpp
    src/client/scenes/widgets/button.cpp
    src/client/scenes/widgets/checkbox.cpp
    src/client/scenes/widgets/textbox.cpp
    src/client/scenes/widgets/text_list.cpp
    src/client/scenes/widgets/chat_ui.cpp
    src/client/scenes/connect.cpp
    src/client/scenes/lobby_list.cpp
    src/client/scenes/lobby.cpp
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
    src/server/game/effects/scenarios.cpp
    src/server/game/effect_holder.cpp
    src/server/game/events.cpp
    src/server/game/game.cpp
    src/server/game/game2.cpp
    src/server/game/make_all_cards.cpp
    src/server/game/player.cpp
)

include_directories(src)
add_subdirectory(resources)

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_executable(bangclient WIN32 ${bangclient_srcs})
else()
    add_executable(bangclient ${bangclient_srcs})
endif()

add_library(network_libs INTERFACE)
target_link_libraries(network_libs INTERFACE PkgConfig::SDL2_net)

target_link_libraries(bangclient network_libs PkgConfig::SDL2 PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx PkgConfig::jsoncpp)
target_link_libraries(bangclient resources_obj locale_headers)
if(WIN32)
    target_sources(bangclient PRIVATE resources/icons/bang.rc)
endif()

find_package(fmt REQUIRED)
if (TARGET fmt::fmt-header-only)
    target_link_libraries(bangclient fmt::fmt-header-only)
else()
    target_link_libraries(bangclient fmt::fmt)
endif()

add_dependencies(bangclient cards_pak)

add_library(bangserverlib SHARED ${bangserver_srcs})
target_link_libraries(bangserverlib network_libs PkgConfig::jsoncpp)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(bangserverlib bang_cards)
    target_compile_definitions(bangserverlib PRIVATE BANG_CARDS_JSON_LINKED)
endif()
target_link_libraries(bangclient bangserverlib)

add_executable(bangserver src/server/main.cpp)
target_link_libraries(bangserver bangserverlib)
if(WIN32 AND MINGW)
    target_link_options(bangserver PRIVATE -mconsole)
endif()