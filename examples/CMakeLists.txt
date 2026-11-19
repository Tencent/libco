cmake_minimum_required(VERSION 2.8)

macro(add_example_target EXAMPLE_TARGET)
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_TARGET} NAME)
    if(${EXAMPLE_TARGET} MATCHES "\\.cpp$")
        string(REPLACE ".cpp" "" EXAMPLE_NAME ${EXAMPLE_NAME})
    endif()

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_TARGET})
    target_include_directories(${EXAMPLE_NAME} PUBLIC ${LIBCO_INCLUDE_DIR})
    target_link_libraries(${EXAMPLE_NAME} PUBLIC colib_static pthread dl)

    add_test(NAME ${EXAMPLE_NAME} COMMAND ${EXAMPLE_NAME})
endmacro()

file(GLOB_RECURSE EXAMPLE_TARGETS "example_*")
foreach(EXAMPLE_TARGET ${EXAMPLE_TARGETS})
    add_example_target(${EXAMPLE_TARGET})
endforeach()