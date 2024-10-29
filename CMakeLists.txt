cmake_minimum_required(VERSION 2.8.12)

message(STATUS "CURRENT_DIR: ${CMAKE_CURRENT_LIST_DIR}")

#set (CMAKE_CXX_FLAGS "-std=c++11 -pthread")
#set (CMAKE_CXX_STANDARD 11)

enable_language(C CXX ASM)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)
set(DIR_LIB_SRCS ${DIR_LIB_SRCS} coctx_swap.S)
# 生成
#add_executable(DSAdmin ${DIR_LIB_SRCS})
add_library (libco STATIC ${DIR_LIB_SRCS})

