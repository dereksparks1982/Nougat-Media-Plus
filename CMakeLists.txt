cmake_minimum_required(VERSION 3.16)
project(NougatMediaSuite VERSION 0.0.38 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REDDMEDIA_P2P_STUB "Build P2P engine without libtorrent for isolated source/UI validation" OFF)
option(REDDMEDIA_AI_STUB "Build deterministic AI test lane without loading llama.cpp" OFF)

find_package(Threads REQUIRED)

add_executable(Nougat_Media_Suite_v38
    src/main.cpp
    src/nougat/nougat_bridge.cpp
    src/diagnostics/diagnostic_engine.cpp
    src/media_server/jellyfin_api_client.cpp
    src/media_server/library_poster.cpp
    src/media_server/library_metadata_cache.cpp
    src/media_server/media_server_manager.cpp
    src/live_tv/tuner_backend.cpp
    src/recommendations/embedding_engine.cpp
    src/recommendations/recommendation_engine.cpp
    src/recommendations/tmdb_client.cpp
    src/recommendations/viewing_history.cpp
    src/recommendations/watch_provider_preferences.cpp
    src/p2p_engine.cpp
    src/p2p_stream_server.cpp
    src/ytdlp_stream_server.cpp
)

target_compile_options(Nougat_Media_Suite_v38 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(Nougat_Media_Suite_v38 PRIVATE X11 dl Threads::Threads)

if(REDDMEDIA_AI_STUB)
    target_compile_definitions(Nougat_Media_Suite_v38 PRIVATE REDDMEDIA_AI_STUB=1)
else()
    set(REDDMEDIA_AI_RUNTIME "${CMAKE_SOURCE_DIR}/components/ai/runtime" CACHE PATH
        "Installed llama.cpp runtime used by Nougat Media Suite")
    find_path(LLAMA_INCLUDE_DIR llama.h
        PATHS "${REDDMEDIA_AI_RUNTIME}/include"
        NO_DEFAULT_PATH REQUIRED)
    find_library(LLAMA_LIBRARY llama
        PATHS "${REDDMEDIA_AI_RUNTIME}/lib" "${REDDMEDIA_AI_RUNTIME}/lib64"
        NO_DEFAULT_PATH REQUIRED)
    target_include_directories(Nougat_Media_Suite_v38 PRIVATE "${LLAMA_INCLUDE_DIR}")
    target_link_libraries(Nougat_Media_Suite_v38 PRIVATE "${LLAMA_LIBRARY}")
    target_link_options(Nougat_Media_Suite_v38 PRIVATE
        "-Wl,--disable-new-dtags"
        "-Wl,-rpath-link,${REDDMEDIA_AI_RUNTIME}/lib"
        "-Wl,-rpath-link,${REDDMEDIA_AI_RUNTIME}/lib64")
    # Keep the root executable relocatable with the project directory.  The
    # accepted v0.0.21 binary embedded the pre-rename absolute project path,
    # which broke libllama.so discovery after ReddMedia became Nougat Media Suite.
    set_target_properties(Nougat_Media_Suite_v38 PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "\$ORIGIN/components/ai/runtime/lib;\$ORIGIN/components/ai/runtime/lib64"
        INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

if(REDDMEDIA_P2P_STUB)
    target_compile_definitions(Nougat_Media_Suite_v38 PRIVATE REDDMEDIA_P2P_STUB=1)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)
    target_link_libraries(Nougat_Media_Suite_v38 PRIVATE PkgConfig::LIBTORRENT)
endif()
