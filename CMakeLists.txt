cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED SDL2 IMPORTED_TARGET)
pkg_check_modules(SDL2_net REQUIRED SDL2_net IMPORTED_TARGET)

find_package(jsoncpp REQUIRED)

set(bangclient_srcs
    src/client/main.cpp
)

set(bangserver_srcs
    src/server/main.cpp
    src/server/read_cards.cpp
    src/server/player.cpp
    src/server/effects/effects.cpp
)

add_subdirectory(resources)

find_package(Boost REQUIRED)

add_executable(bangclient ${bangclient_srcs})
target_link_libraries(bangclient PkgConfig::SDL2 jsoncpp_static)

add_executable(bangserver ${bangserver_srcs})
target_link_libraries(bangserver jsoncpp_static resources)