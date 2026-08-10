cmake_minimum_required(VERSION 3.16)
project(ReddMedia VERSION 0.0.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ReddMedia_v9 src/main.cpp)
target_compile_options(ReddMedia_v9 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(ReddMedia_v9 PRIVATE X11 dl)
