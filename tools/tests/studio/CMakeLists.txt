cmake_minimum_required(VERSION 3.16)
project(NougatStudioCoreTests LANGUAGES CXX)

# Independent Studio core validation. This is NOT a Nougat application build,
# install target, GitHub Actions workflow, or automatically executed test.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)
enable_testing()

get_filename_component(NMP_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_library(nougat_studio_core STATIC
    "${NMP_SOURCE_ROOT}/src/studio/media_job_engine.cpp"
    "${NMP_SOURCE_ROOT}/src/studio/media_inspector.cpp"
    "${NMP_SOURCE_ROOT}/src/studio/studio_service.cpp"
    "${NMP_SOURCE_ROOT}/src/studio/folder_batch_planner.cpp"
)
target_include_directories(nougat_studio_core PUBLIC "${NMP_SOURCE_ROOT}/src")
target_link_libraries(nougat_studio_core PUBLIC Threads::Threads)
target_compile_options(nougat_studio_core PRIVATE -Wall -Wextra -Werror)

foreach(test IN ITEMS
    test_studio_media_job_engine
    test_studio_media_inspector
    test_studio_fake_ffmpeg
    test_studio_folder_batch
    test_studio_queue_controls
)
    add_executable("${test}" "${NMP_SOURCE_ROOT}/tools/tests/${test}.cpp")
    target_link_libraries("${test}" PRIVATE nougat_studio_core)
    target_compile_options("${test}" PRIVATE -Wall -Wextra -Werror)
    add_test(NAME "${test}" COMMAND "${test}")
endforeach()
