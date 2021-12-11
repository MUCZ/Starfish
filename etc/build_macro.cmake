
macro (add_exec _name)
    add_executable ("${_name}" "${_name}.cc")
    target_link_libraries("${_name}" ${ARGN} starfish pthread)
endmacro(add_exec)

