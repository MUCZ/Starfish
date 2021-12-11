cmake_minimum_required(VERSION 3.8)

project(Starfish)

# macros 
# include (etc/build_macro.cmake)


macro (add_exec _name)
    add_executable ("${_name}" "${_name}.cc")
    target_link_libraries("${_name}" ${ARGN} starfish pthread)
endmacro(add_exec)

# flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -pedantic -pedantic-errors -Werror -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Weffc++ -Wold-style-cast")

# include 
include_directories("${PROJECT_SOURCE_DIR}/lib/util")
include_directories("${PROJECT_SOURCE_DIR}/lib/tcp_helpers")
include_directories("${PROJECT_SOURCE_DIR}/lib")

# subs
add_subdirectory("${PROJECT_SOURCE_DIR}/lib")

add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

# add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

