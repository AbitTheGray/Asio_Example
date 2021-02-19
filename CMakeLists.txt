cmake_minimum_required(VERSION 3.16)
project(Asio_Example CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_policy(SET CMP0077 NEW) # set() overrides option()



#--------------------------------
# Compiler configuration
#--------------------------------

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>") # Define DEBUG in Debug builds

if(MINGW AND UNIX) # Enable cross-compilation from Linux for Windows
    message(NOTICE "MinGW detected, setting compilation for windows")
    set(WIN32 ON)
endif()



#--------------------------------
# External libraries
#--------------------------------

add_subdirectory(lib)



#--------------------------------
# Source
#--------------------------------

file(GLOB_RECURSE PROJECT_SOURCE src/*.hpp src/*.cpp src/*.h src/*.c)

add_executable(Asio_Example ${PROJECT_SOURCE})
target_include_directories(Asio_Example PUBLIC src/)

set_target_properties(Asio_Example PROPERTIES OUTPUT_NAME "server")

target_link_libraries(Asio_Example asio)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC
    target_link_libraries(Asio_Example -static-libgcc -static-libstdc++)
    target_compile_options(Asio_Example PUBLIC -fcoroutines)
    if(WIN32)
        target_link_libraries(Asio_Example ws2_32 wsock32 -static winpthread)
    endif()
    if(UNIX)
        target_link_libraries(Asio_Example pthread)
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()
