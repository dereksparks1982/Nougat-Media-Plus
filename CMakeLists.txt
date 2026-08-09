cmake_minimum_required(VERSION 3.16)
project(ReddMedia VERSION 0.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ReddMedia src/main.cpp)
target_compile_options(ReddMedia PRIVATE -Wall -Wextra -Werror)
target_link_libraries(ReddMedia PRIVATE X11 dl)
