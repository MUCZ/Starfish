cmake_minimum_required(VERSION 3.8)

project(Starfish)

# macros 
include (etc/build_macro.cmake)

# flags
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++17 -g -ggdb -pedantic -pedantic-errors -Werror -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Weffc++ -Wold-style-cast")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -Wall" ) 

# include 
include_directories("${PROJECT_SOURCE_DIR}/lib/util")
include_directories("${PROJECT_SOURCE_DIR}/lib/tcp_helpers")
include_directories("${PROJECT_SOURCE_DIR}/lib")

# subs
add_subdirectory("${PROJECT_SOURCE_DIR}/lib")

add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

# add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

