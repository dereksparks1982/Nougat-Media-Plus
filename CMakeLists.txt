cmake_minimum_required(VERSION 3.16)
project(NougatMediaPlus VERSION 0.0.65 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REDDMEDIA_P2P_STUB "Build P2P engine without libtorrent for isolated source/UI validation" OFF)
option(REDDMEDIA_AI_STUB "Build deterministic AI test lane without loading llama.cpp" OFF)

find_package(Threads REQUIRED)

add_executable(Nougat_Media_Plus_v65
    src/main.cpp
    src/network_center.cpp
    src/satellite_center.cpp
    components/satellite/sgp4/sgp4.c
    src/radio/radio_backend.cpp
    src/chat/nougat_chat.cpp
    src/games/emulator_host.cpp
    src/nougat/nougat_bridge.cpp
    src/search/secure_search.cpp
    src/privacy/privacy_policy.cpp
    src/privacy/privacy_receipt.cpp
    src/privacy/privacy_broker_client.cpp
    src/crawler/crawler_access_manager.cpp
    src/security/scanner_process.cpp
    src/lan/lan_media_service.cpp
    src/lan/lan_viewer_service.cpp
    src/safety/child_safe_controls.cpp
    src/security/security_advisory_service.cpp
    src/alerts/public_safety_alerts.cpp
    src/world_tv/world_tv_service.cpp
    src/diagnostics/diagnostic_engine.cpp
    src/media_server/jellyfin_api_client.cpp
    src/media_server/library_poster.cpp
    src/media_server/library_metadata_cache.cpp
    src/media_server/media_server_manager.cpp
    src/live_tv/tuner_backend.cpp
    src/live_tv/hdhomerun_provider.cpp
    src/recommendations/embedding_engine.cpp
    src/recommendations/recommendation_engine.cpp
    src/recommendations/tmdb_client.cpp
    src/recommendations/viewing_history.cpp
    src/recommendations/watch_provider_preferences.cpp
    src/p2p_engine.cpp
    src/p2p_stream_server.cpp
    src/ytdlp_stream_server.cpp
)

target_compile_options(Nougat_Media_Plus_v65 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(Nougat_Media_Plus_v65 PRIVATE X11 dl Threads::Threads m)

if(REDDMEDIA_AI_STUB)
    target_compile_definitions(Nougat_Media_Plus_v65 PRIVATE REDDMEDIA_AI_STUB=1)
else()
    set(REDDMEDIA_AI_RUNTIME "${CMAKE_SOURCE_DIR}/components/ai/runtime" CACHE PATH
        "Installed llama.cpp runtime used by Nougat Media Suite")
    find_path(LLAMA_INCLUDE_DIR llama.h
        PATHS "${REDDMEDIA_AI_RUNTIME}/include"
        NO_DEFAULT_PATH REQUIRED)
    find_library(LLAMA_LIBRARY llama
        PATHS "${REDDMEDIA_AI_RUNTIME}/lib" "${REDDMEDIA_AI_RUNTIME}/lib64"
        NO_DEFAULT_PATH REQUIRED)
    target_include_directories(Nougat_Media_Plus_v65 PRIVATE "${LLAMA_INCLUDE_DIR}")
    target_link_libraries(Nougat_Media_Plus_v65 PRIVATE "${LLAMA_LIBRARY}")
    target_link_options(Nougat_Media_Plus_v65 PRIVATE
        "-Wl,--disable-new-dtags"
        "-Wl,-rpath-link,${REDDMEDIA_AI_RUNTIME}/lib"
        "-Wl,-rpath-link,${REDDMEDIA_AI_RUNTIME}/lib64")
    set_target_properties(Nougat_Media_Plus_v65 PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "\$ORIGIN/components/ai/runtime/lib;\$ORIGIN/components/ai/runtime/lib64"
        INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

if(REDDMEDIA_P2P_STUB)
    target_compile_definitions(Nougat_Media_Plus_v65 PRIVATE REDDMEDIA_P2P_STUB=1)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)
    target_link_libraries(Nougat_Media_Plus_v65 PRIVATE PkgConfig::LIBTORRENT)
endif()

add_executable(nougat_web_service_v55
    src/lan/web_player_daemon.cpp
    src/lan/lan_media_service.cpp
    src/media_server/jellyfin_api_client.cpp
    src/media_server/library_poster.cpp
)
target_compile_options(nougat_web_service_v55 PRIVATE -Wall -Wextra -Werror)
target_link_libraries(nougat_web_service_v55 PRIVATE Threads::Threads)
set_target_properties(nougat_web_service_v55 PROPERTIES OUTPUT_NAME "nougat-web-service")
