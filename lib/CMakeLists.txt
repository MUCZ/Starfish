file(GLOB LIB_SOURCES "*.cc" "util/*.cc" "tcp_helpers/*.cc")

add_library(starfish STATIC ${LIB_SOURCES})