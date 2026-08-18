cmake_minimum_required(VERSION 3.16)
project(ReddMedia VERSION 0.0.15 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REDDMEDIA_P2P_STUB "Build P2P engine without libtorrent for isolated source/UI validation" OFF)

find_package(Threads REQUIRED)

add_executable(ReddMedia_v15
    src/main.cpp
    src/media_server/jellyfin_api_client.cpp
    src/media_server/media_server_manager.cpp
    src/p2p_engine.cpp
    src/p2p_stream_server.cpp
    src/ytdlp_stream_server.cpp
)

target_compile_options(ReddMedia_v15 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(ReddMedia_v15 PRIVATE X11 dl Threads::Threads)

if(REDDMEDIA_P2P_STUB)
    target_compile_definitions(ReddMedia_v15 PRIVATE REDDMEDIA_P2P_STUB=1)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)
    target_link_libraries(ReddMedia_v15 PRIVATE PkgConfig::LIBTORRENT)
endif()
