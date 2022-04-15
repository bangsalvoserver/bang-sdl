cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf IMPORTED_TARGET)
pkg_check_modules(SDL2_image REQUIRED SDL2_image IMPORTED_TARGET)
pkg_check_modules(SDL2_gfx REQUIRED SDL2_gfx IMPORTED_TARGET)

find_package(Boost REQUIRED COMPONENTS system)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(asio INTERFACE ${Boost_LIBRARIES})
if(MINGW)
    target_link_libraries(asio INTERFACE wsock32 ws2_32)
endif()

pkg_check_modules(jsoncpp REQUIRED jsoncpp IMPORTED_TARGET)

find_package(fmt REQUIRED)
if (TARGET fmt::fmt-header-only)
    link_libraries(fmt::fmt-header-only)
else()
    link_libraries(fmt::fmt)
endif()

set(bangclient_srcs
    src/client/intl.cpp
    src/client/entrypoint.cpp
    src/client/manager.cpp
    src/client/user_info.cpp
    src/client/chat_ui.cpp
    src/client/media_pak.cpp
    src/client/os_api.cpp
    src/client/game/animations.cpp
    src/client/game/card.cpp
    src/client/game/player.cpp
    src/client/game/game.cpp
    src/client/game/game_ui.cpp
    src/client/game/game_message_box.cpp
    src/client/game/target_finder.cpp
    src/client/scenes/connect.cpp
    src/client/scenes/loading.cpp
    src/client/scenes/lobby_list.cpp
    src/client/scenes/lobby.cpp
    src/client/widgets/button.cpp
    src/client/widgets/checkbox.cpp
    src/client/widgets/textbox.cpp
    src/client/widgets/text_list.cpp
    src/client/widgets/profile_pic.cpp
)

set(banggame_srcs
    src/game/effects/base/effects.cpp
    src/game/effects/base/equips.cpp
    src/game/effects/base/characters.cpp
    src/game/effects/base/requests.cpp
    src/game/effects/dodgecity/characters.cpp
    src/game/effects/highnoon/scenarios.cpp
    src/game/effects/fistfulofcards/scenarios.cpp
    src/game/effects/goldrush/effects.cpp
    src/game/effects/goldrush/equips.cpp
    src/game/effects/goldrush/characters.cpp
    src/game/effects/valleyofshadows/effects.cpp
    src/game/effects/valleyofshadows/equips.cpp
    src/game/effects/valleyofshadows/characters.cpp
    src/game/effects/valleyofshadows/requests.cpp
    src/game/effects/wildwestshow/characters.cpp
    src/game/effects/armedanddangerous/equips.cpp
    src/game/effects/armedanddangerous/effects.cpp
    src/game/effects/armedanddangerous/characters.cpp
    src/game/effects/armedanddangerous/requests.cpp
    src/game/effects/canyondiablo/effects.cpp
    src/game/effects/canyondiablo/equips.cpp
    src/game/holders.cpp
    src/game/game.cpp
    src/game/game_table.cpp
    src/game/make_all_cards.cpp
    src/game/draw_check_handler.cpp
    src/game/player.cpp
    src/game/possible_to_play.cpp
)

set(bangnet_srcs
    src/server/server.cpp
    src/server/manager.cpp
    src/server/ansicvt.cpp
)

include_directories(src)
add_subdirectory(resources)

add_library(bangclient SHARED ${bangclient_srcs})
target_compile_definitions(bangclient PRIVATE SDL_MAIN_HANDLED)

include(GenerateExportHeader)
generate_export_header(bangclient
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/src/client/bangclient_export.h"
)

target_link_libraries(bangclient PkgConfig::SDL2 PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx PkgConfig::jsoncpp)

add_dependencies(bangclient cards_pak media_pak)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(bangclient game_options)
endif()

add_library(banggame SHARED ${banggame_srcs})
target_link_libraries(banggame PkgConfig::jsoncpp)

add_library(bangnet SHARED ${bangnet_srcs})
target_link_libraries(bangnet asio banggame)

target_link_libraries(bangclient bangnet)

add_executable(bangserver src/server/main.cpp)
target_link_libraries(bangserver bangnet)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (WIN32)
        add_executable(banglauncher WIN32 src/client/main.c resources/bang.rc)
        if (MINGW)
            target_link_options(banglauncher PRIVATE -mconsole)
        endif()
    else()
        add_executable(banglauncher src/client/main.c)
    endif()

    target_link_libraries(banglauncher bangclient)
endif()