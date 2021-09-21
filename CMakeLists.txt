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
    src/server/manager.cpp
    src/server/game/card.cpp
    src/server/game/effects.cpp
    src/server/game/game.cpp
    src/server/game/player.cpp
    src/server/game/responses.cpp
)

add_subdirectory(resources)

find_package(Boost REQUIRED)

add_executable(bangclient ${bangclient_srcs})
target_link_libraries(bangclient PkgConfig::SDL2 jsoncpp_static)
target_include_directories(bangclient PUBLIC src)

add_executable(bangserver ${bangserver_srcs})
target_include_directories(bangserver PUBLIC src)
target_link_libraries(bangserver PkgConfig::SDL2_net jsoncpp_static bang_cards)
target_link_options(bangserver PRIVATE -mconsole)

add_executable(bangservertest src/server/test.cpp)
target_include_directories(bangservertest PUBLIC src)
target_link_libraries(bangservertest PkgConfig::SDL2_net jsoncpp_static bang_cards)
target_link_options(bangservertest PRIVATE -mconsole)