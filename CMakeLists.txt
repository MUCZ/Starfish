cmake_minimum_required(VERSION 3.8)

project(Starfish)

# macros 
include (etc/build_macro.cmake)

# build falgs
include (etc/flags.cmake)

# include 
include_directories("${PROJECT_SOURCE_DIR}/lib/util")
include_directories("${PROJECT_SOURCE_DIR}/lib/tcp_helpers")
include_directories("${PROJECT_SOURCE_DIR}/lib")

# subs
add_subdirectory("${PROJECT_SOURCE_DIR}/lib")

add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

include (etc/tests.cmake)
