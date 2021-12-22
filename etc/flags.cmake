# flags
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++17 -g -ggdb -pedantic -pedantic-errors -Werror -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Weffc++ -Wold-style-cast")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -Wall" ) 