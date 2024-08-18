cmake_minimum_required(VERSION 3.13.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if (MSVC)
        add_compile_options(/bigobj)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    elseif(MINGW)
        add_compile_options(-fno-ms-extensions)
    endif()
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

# external dependencies

add_library(banglibs INTERFACE)

# sdl2

add_library(sdl2_libraries INTERFACE)
if (MSVC)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2-gfx REQUIRED)
    find_package(SDL2_mixer REQUIRED)

    target_link_libraries(sdl2_libraries INTERFACE SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image::SDL2_image SDL2::SDL2_gfx SDL2_mixer::SDL2_mixer)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
    pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf IMPORTED_TARGET)
    pkg_check_modules(SDL2_image REQUIRED SDL2_image IMPORTED_TARGET)
    pkg_check_modules(SDL2_gfx REQUIRED SDL2_gfx IMPORTED_TARGET)
    pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer IMPORTED_TARGET)

    target_link_libraries(sdl2_libraries INTERFACE PkgConfig::SDL2 PkgConfig::SDL2_ttf PkgConfig::SDL2_image PkgConfig::SDL2_gfx PkgConfig::SDL2_mixer)
endif()

target_link_libraries(banglibs INTERFACE sdl2_libraries)

# asio

target_compile_definitions(banglibs INTERFACE ASIO_STANDALONE)

if(MINGW)
    target_link_libraries(banglibs INTERFACE wsock32 ws2_32)
endif()

# openssl

find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
    message(WARNING "Could not find OpenSSL")
    target_compile_definitions(banglibs INTERFACE WSCLIENT_NO_TLS)
else()
    target_link_libraries(banglibs INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif()

# websocketpp

target_include_directories(banglibs INTERFACE external/websocketpp)
target_compile_definitions(banglibs INTERFACE _WEBSOCKETPP_CPP11_STL_)
target_compile_definitions(banglibs INTERFACE _WEBSOCKETPP_CPP11_THREAD_)

# range-v3

find_package(range-v3 REQUIRED)
target_link_libraries(banglibs INTERFACE range-v3)

# nlohmann-json

find_package(nlohmann_json 3.11.0 REQUIRED)
target_link_libraries(banglibs INTERFACE nlohmann_json::nlohmann_json)

# reflect

target_include_directories(banglibs INTERFACE external/reflect)
target_compile_definitions(banglibs INTERFACE NTEST)

# bang client dll

add_library(bangclient SHARED)

target_link_libraries(bangclient PRIVATE banglibs)

add_subdirectory(resources)
add_subdirectory(src)

target_include_directories(bangclient PRIVATE src)

target_compile_definitions(bangclient PRIVATE SDL_MAIN_HANDLED)

include(GenerateExportHeader)
generate_export_header(bangclient
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/exports/bangclient_export.h"
)

target_include_directories(bangclient PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/exports")

add_dependencies(bangclient cards_pak media_pak sounds_pak)

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
        add_executable(banglauncher WIN32 src/main.c)
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
