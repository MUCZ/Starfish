# binary lib of check harness
add_library (check_lib STATIC
            send_equivalence_checker.cc
            tcp_fsm_test_harness.cc
            byte_stream_test_harness.cc)

macro (add_test_exec exec_name)
    add_executable ("${exec_name}" "${exec_name}.cc")
    target_link_libraries("${exec_name}" check_lib ${ARGN})
    target_link_libraries("${exec_name}" starfish ${ARGN})
endmacro (add_test_exec)

# tests
add_test_exec (send_connect)
add_test_exec (recv_connect)
add_test_exec (fsm_stream_reassembler_single)
add_test_exec (fsm_winsize)