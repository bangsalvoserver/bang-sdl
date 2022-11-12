cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/bigobj)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(bangclient_srcs
    src/config.cpp
    src/image_serial.cpp
    src/intl.cpp
    src/entrypoint.cpp
    src/manager.cpp
    src/chat_ui.cpp
    src/media_pak.cpp
    src/sounds_pak.cpp
    src/os_api.cpp
    src/gamescene/animations.cpp
    src/gamescene/card.cpp
    src/gamescene/card_serial.cpp
    src/gamescene/player.cpp
    src/gamescene/filters.cpp
    src/gamescene/game.cpp
    src/gamescene/game_ui.cpp
    src/gamescene/game_message_box.cpp
    src/gamescene/target_finder.cpp
    src/gamescene/options.cpp
    src/scenes/connect.cpp
    src/scenes/loading.cpp
    src/scenes/lobby_list.cpp
    src/scenes/lobby.cpp
    src/widgets/button.cpp
    src/widgets/checkbox.cpp
    src/widgets/textbox.cpp
    src/widgets/text_list.cpp
    src/widgets/profile_pic.cpp
)

add_library(sdl2_libraries INTERFACE)
if (MSVC)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2-image REQUIRED)
    find_package(SDL2-gfx REQUIRED)

    target_link_libraries(sdl2_libraries INTERFACE SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2::SDL2_image SDL2::SDL2_gfx)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
    pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf IMPORTED_TARGET)
    pkg_check_modules(SDL2_image REQUIRED SDL2_image IMPORTED_TARGET)
    pkg_check_modules(SDL2_gfx REQUIRED SDL2_gfx IMPORTED_TARGET)

    target_link_libraries(sdl2_libraries INTERFACE PkgConfig::SDL2 PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx)
endif()

add_subdirectory(game)
add_subdirectory(resources)
add_subdirectory(external/tiny-process-library)

set_target_properties(tiny-process-library PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

add_library(bangclient SHARED ${bangclient_srcs})
target_include_directories(bangclient PRIVATE src)

target_compile_definitions(bangclient PRIVATE SDL_MAIN_HANDLED)

if(DEFINED OFFICIAL_BANG_SERVER)
    target_compile_definitions(bangclient PRIVATE "OFFICIAL_BANG_SERVER=\"${OFFICIAL_BANG_SERVER}\"")
endif()

include(GenerateExportHeader)
generate_export_header(bangclient
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/exports/bangclient_export.h"
)

target_include_directories(bangclient PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/exports")

target_link_libraries(bangclient PUBLIC bangcommon sdl2_libraries tiny-process-library)
target_compile_definitions(bangclient PRIVATE BUILD_BANG_CLIENT)

add_dependencies(bangclient cards_pak media_pak sounds_pak)

set_target_properties(bangclient bangserver tiny-process-library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(BUILD_LAUNCHER "Build Launcher" ON)

if (BUILD_GAME_RELEASE)
    set(GIT_CLIENT_VERSION_C "${CMAKE_CURRENT_BINARY_DIR}/git_client_version.c")
    set(GIT_CARDS_VERSION_C "${CMAKE_CURRENT_BINARY_DIR}/git_cards_version.c")

    set(PRE_CONFIGURE_FILE .cmake/git_client_version.c.in)
    set(POST_CONFIGURE_FILE "${GIT_CLIENT_VERSION_C}")
    set(GIT_WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GIT_STATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/git-state-hash-client")
    set(CHECK_GIT_TARGET_NAME check_git_client)
    include(game/git_watcher.cmake)

    set(PRE_CONFIGURE_FILE .cmake/git_cards_version.c.in)
    set(POST_CONFIGURE_FILE "${GIT_CARDS_VERSION_C}")
    set(GIT_WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/cards")
    set(GIT_STATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/git-state-hash-cards")
    set(CHECK_GIT_TARGET_NAME check_git_cards)
    include(game/git_watcher.cmake)

    add_library(git_client_version STATIC "${GIT_CLIENT_VERSION_C}" "${GIT_CARDS_VERSION_C}")
    add_dependencies(git_client_version check_git_client check_git_cards)
    target_compile_definitions(git_client_version PUBLIC HAVE_GIT_CLIENT_VERSION)

    target_link_libraries(bangclient PUBLIC git_client_version)
endif()

if (BUILD_LAUNCHER)
    if (WIN32)
        add_executable(banglauncher WIN32 src/main.c resources/bang.rc)
        if (MINGW)
            target_link_options(banglauncher PRIVATE -mconsole)
        elseif(MSVC)
            target_link_options(banglauncher PRIVATE /SUBSYSTEM:CONSOLE)
        endif()
    else()
        add_executable(banglauncher src/main.c)
    endif()

    target_link_libraries(banglauncher bangclient)
    set_target_properties(banglauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
