# Unit tests

# enable CTest testing
enable_testing ()

# add tests
add_test(NAME t_send_connect        COMMAND send_connect)
add_test(NAME t_recv_connect        COMMAND recv_connect)
add_test(NAME t_strm_reassem_single COMMAND fsm_stream_reassembler_single)
add_test(NAME t_winsize             COMMAND fsm_winsize)

# `make check` = check all tests
add_custom_target ( check COMMAND "${PROJECT_SOURCE_DIR}/tun.sh" stop
                          COMMAND "${PROJECT_SOURCE_DIR}/tun.sh" start
                          COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 10 -R 't_'
                          COMMENT "Testing...")