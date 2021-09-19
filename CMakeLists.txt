cmake_minimum_required(VERSION 3.6.0)
project(bangcardgame VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(bangclient_srcs
src/client/main.cpp
)

add_executable(bangclient ${bangclient_srcs})
target_link_libraries(bangclient ${SDL2_LIBRARIES})