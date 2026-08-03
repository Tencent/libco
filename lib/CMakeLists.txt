cmake_minimum_required(VERSION 2.8)

# Use c and asm
enable_language(C ASM)

# Add source files
set(SOURCE_FILES
        co_epoll.cpp
        co_hook_sys_call.cpp
        co_routine.cpp
        coctx.cpp
        co_comm.cpp
        coctx_swap.S)

# Add static and shared library target
add_library(colib_static STATIC ${SOURCE_FILES})
add_library(colib_shared SHARED ${SOURCE_FILES})

target_compile_definitions(colib_static PRIVATE _GNU_SOURCE _REENTRANT)
target_compile_definitions(colib_shared PRIVATE _GNU_SOURCE _REENTRANT)

# Set library output name
set_target_properties(colib_static PROPERTIES OUTPUT_NAME colib)
set_target_properties(colib_shared PROPERTIES OUTPUT_NAME colib)

set_target_properties(colib_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(colib_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Set shared library version, will generate libcolib.${LIBCO_VERSION}.so and a symbol link named libcolib.so
# For mac osx, the extension name will be .dylib
set_target_properties(colib_shared PROPERTIES VERSION ${LIBCO_VERSION} SOVERSION ${LIBCO_VERSION})