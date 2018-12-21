cmake_minimum_required(VERSION 2.8)
project(libco)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set lib version
set(LIBCO_VERSION   0.5)

# Set cflags
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -fno-strict-aliasing -O2 -Wall -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64")

# Use c and asm
enable_language(C ASM)

# Add source files
if (DEFINED __CYGWIN__)
    include_directories(${PROJECT_SOURCE_DIR}/port_to_cygwin)
    add_library(epoll_lib ${PROJECT_SOURCE_DIR}/port_to_cygwin/sys/epoll.cpp)

    set(SOURCE_FILES
        co_epoll.cpp
        co_hook_sys_call.cpp
        co_routine.cpp
        coctx.cpp
        ${PROJECT_SOURCE_DIR}/port_to_cygwin/sys/epoll.cpp
        coctx_swap.S)
else()
    set(SOURCE_FILES
            co_epoll.cpp
            co_hook_sys_call.cpp
            co_routine.cpp
            coctx.cpp
            coctx_swap.S)
endif()

# Add static and shared library target
add_library(colib_static STATIC ${SOURCE_FILES})
add_library(colib_shared SHARED ${SOURCE_FILES})

# Set library output name
set_target_properties(colib_static PROPERTIES OUTPUT_NAME colib)
set_target_properties(colib_shared PROPERTIES OUTPUT_NAME colib)

set_target_properties(colib_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(colib_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Set shared library version, will generate libcolib.${LIBCO_VERSION}.so and a symbol link named libcolib.so
# For mac osx, the extension name will be .dylib
set_target_properties(colib_shared PROPERTIES VERSION ${LIBCO_VERSION} SOVERSION ${LIBCO_VERSION})



# Macro for add example target
macro(add_example_target EXAMPLE_TARGET)
    add_executable("example_${EXAMPLE_TARGET}" "example_${EXAMPLE_TARGET}.cpp")
    target_link_libraries("example_${EXAMPLE_TARGET}" colib_static pthread dl)
endmacro(add_example_target)

add_example_target(closure)
add_example_target(cond)
add_example_target(copystack)
add_example_target(echocli)
add_example_target(echosvr)
add_example_target(poll)
add_example_target(setenv)
add_example_target(specific)
add_example_target(thread)

if (DEFINED __CYGWIN__)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fpermissive)
    include_directories(${PROJECT_SOURCE_DIR})

    add_executable(cm_routine_test
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/cm_routine_test.cpp
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/cm_routine.cpp)

    add_executable(co_routine_test
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/co_routine_test.cpp)
    target_link_libraries(co_routine_test colib_shared)

    add_executable(epoll_test
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/sys/epoll_test.cpp
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/sys/epoll.cpp)

    add_executable(getThreadId_test
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/getThreadId_test.cpp)

    add_executable(ucontext_test
            ${PROJECT_SOURCE_DIR}/port_to_cygwin/ucontext_test.cpp)
endif()