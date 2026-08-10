cmake_minimum_required(VERSION 3.16)
project(ReddMedia VERSION 0.0.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ReddMedia_v8 src/main.cpp)
target_compile_options(ReddMedia_v8 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(ReddMedia_v8 PRIVATE X11 dl)
