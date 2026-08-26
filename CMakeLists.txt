cmake_minimum_required(VERSION 3.16)
project(NougatMediaSuite VERSION 0.0.50 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Backward-compatible switch retained for isolated validation jobs.
option(REDDMEDIA_P2P_STUB "Build P2P engine without libtorrent for isolated source/UI validation" OFF)
set(NOUGAT_AI_MODE "AUTO" CACHE STRING "Local AI build mode: AUTO, ENABLED, or STUB")
set_property(CACHE NOUGAT_AI_MODE PROPERTY STRINGS AUTO ENABLED STUB)
set(NOUGAT_P2P_MODE "AUTO" CACHE STRING "P2P build mode: AUTO, ENABLED, or STUB")
set_property(CACHE NOUGAT_P2P_MODE PROPERTY STRINGS AUTO ENABLED STUB)

find_package(Threads REQUIRED)

add_executable(Nougat_Media_Suite_v50
    src/main.cpp
    src/platform/nougat_paths.cpp
    src/workshop/split_archive_service.cpp
    src/games/emulator_host.cpp
    src/nougat/nougat_bridge.cpp
    src/search/secure_search.cpp
    src/privacy/privacy_policy.cpp
    src/privacy/privacy_receipt.cpp
    src/privacy/privacy_broker_client.cpp
    src/crawler/crawler_access_manager.cpp
    src/security/scanner_process.cpp
    src/lan/lan_media_service.cpp
    src/world_tv/world_tv_service.cpp
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

target_compile_options(Nougat_Media_Suite_v50 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(Nougat_Media_Suite_v50 PRIVATE X11 dl Threads::Threads)

# Local AI is a managed optional component in v0.0.50. The source tree no
# longer owns a downloaded llama.cpp runtime. AUTO uses a managed runtime when
# present and otherwise compiles the deterministic stub so the core app still
# builds and runs without Local AI installed.
if(DEFINED ENV{XDG_DATA_HOME} AND NOT "$ENV{XDG_DATA_HOME}" STREQUAL "")
    set(_NOUGAT_DATA_HOME "$ENV{XDG_DATA_HOME}")
elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    set(_NOUGAT_DATA_HOME "$ENV{HOME}/.local/share")
else()
    set(_NOUGAT_DATA_HOME "${CMAKE_BINARY_DIR}/nougat-data")
endif()
set(NOUGAT_AI_RUNTIME "${_NOUGAT_DATA_HOME}/nougat/runtime/llama.cpp" CACHE PATH
    "Managed llama.cpp runtime used by Nougat Media Suite")

string(TOUPPER "${NOUGAT_AI_MODE}" _NOUGAT_AI_MODE)
if(NOT _NOUGAT_AI_MODE MATCHES "^(AUTO|ENABLED|STUB)$")
    message(FATAL_ERROR "NOUGAT_AI_MODE must be AUTO, ENABLED, or STUB")
endif()

set(_NOUGAT_USE_AI_STUB FALSE)
if(_NOUGAT_AI_MODE STREQUAL "STUB")
    set(_NOUGAT_USE_AI_STUB TRUE)
else()
    find_path(LLAMA_INCLUDE_DIR llama.h
        PATHS "${NOUGAT_AI_RUNTIME}/include"
        NO_DEFAULT_PATH)
    find_library(LLAMA_LIBRARY llama
        PATHS "${NOUGAT_AI_RUNTIME}/lib" "${NOUGAT_AI_RUNTIME}/lib64"
        NO_DEFAULT_PATH)
    if(NOT LLAMA_INCLUDE_DIR OR NOT LLAMA_LIBRARY)
        if(_NOUGAT_AI_MODE STREQUAL "ENABLED")
            message(FATAL_ERROR
                "Local AI was required but the managed llama.cpp runtime is incomplete at ${NOUGAT_AI_RUNTIME}")
        endif()
        set(_NOUGAT_USE_AI_STUB TRUE)
        message(STATUS "Managed Local AI runtime not installed; building Local AI stub")
    endif()
endif()

if(_NOUGAT_USE_AI_STUB)
    target_compile_definitions(Nougat_Media_Suite_v50 PRIVATE REDDMEDIA_AI_STUB=1)
else()
    target_include_directories(Nougat_Media_Suite_v50 PRIVATE "${LLAMA_INCLUDE_DIR}")
    target_link_libraries(Nougat_Media_Suite_v50 PRIVATE "${LLAMA_LIBRARY}")
    target_link_options(Nougat_Media_Suite_v50 PRIVATE
        "-Wl,--disable-new-dtags"
        "-Wl,-rpath-link,${NOUGAT_AI_RUNTIME}/lib"
        "-Wl,-rpath-link,${NOUGAT_AI_RUNTIME}/lib64")
    set_target_properties(Nougat_Media_Suite_v50 PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "${NOUGAT_AI_RUNTIME}/lib;${NOUGAT_AI_RUNTIME}/lib64"
        INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# P2P is also optional to the core application. AUTO enables libtorrent when
# the development package is available and otherwise compiles the explicit
# stub. ENABLED is for release builders who want absence to be a hard failure.
if(REDDMEDIA_P2P_STUB)
    set(NOUGAT_P2P_MODE "STUB")
endif()
string(TOUPPER "${NOUGAT_P2P_MODE}" _NOUGAT_P2P_MODE)
if(NOT _NOUGAT_P2P_MODE MATCHES "^(AUTO|ENABLED|STUB)$")
    message(FATAL_ERROR "NOUGAT_P2P_MODE must be AUTO, ENABLED, or STUB")
endif()

set(_NOUGAT_USE_P2P_STUB FALSE)
if(_NOUGAT_P2P_MODE STREQUAL "STUB")
    set(_NOUGAT_USE_P2P_STUB TRUE)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBTORRENT QUIET IMPORTED_TARGET libtorrent-rasterbar)
    endif()
    if(NOT LIBTORRENT_FOUND)
        if(_NOUGAT_P2P_MODE STREQUAL "ENABLED")
            message(FATAL_ERROR "P2P was required but libtorrent-rasterbar development files were not found")
        endif()
        set(_NOUGAT_USE_P2P_STUB TRUE)
        message(STATUS "libtorrent-rasterbar not installed; building P2P stub")
    endif()
endif()

if(_NOUGAT_USE_P2P_STUB)
    target_compile_definitions(Nougat_Media_Suite_v50 PRIVATE REDDMEDIA_P2P_STUB=1)
else()
    target_link_libraries(Nougat_Media_Suite_v50 PRIVATE PkgConfig::LIBTORRENT)
endif()
