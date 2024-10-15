cmake_minimum_required(VERSION 3.14)

project(CHIP8)

# add cmake-specific flag to disable 'C with C++ compiler' deprecation warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated")
endif()

# set compiler language
set(CH_LANGUAGE C)

# compile libraries

# compile applications
add_subdirectory(app/executor)
