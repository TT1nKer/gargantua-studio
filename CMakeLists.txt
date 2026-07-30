cmake_minimum_required(VERSION 3.20)

project(GargantuaStudio VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(cmake/SolarDependency.cmake)

option(GARGANTUA_BUILD_TESTS "Build Gargantua tests" ON)

function(gargantua_enable_warnings target)
    if(MSVC)
        target_compile_options("${target}" PRIVATE /W4)
    else()
        target_compile_options(
            "${target}" PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

execute_process(
    COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse HEAD
    RESULT_VARIABLE gargantua_git_result
    OUTPUT_VARIABLE GARGANTUA_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(LENGTH "${GARGANTUA_GIT_COMMIT}" gargantua_commit_length)
if(
    NOT gargantua_git_result EQUAL 0 OR
    NOT gargantua_commit_length EQUAL 40 OR
    NOT GARGANTUA_GIT_COMMIT MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "Gargantua source has no valid Git commit")
endif()
execute_process(
    COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" status --porcelain
    OUTPUT_VARIABLE gargantua_git_status
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(gargantua_git_status STREQUAL "")
    set(GARGANTUA_BUILD_DIRTY 0)
else()
    set(GARGANTUA_BUILD_DIRTY 1)
endif()

add_library(
    gargantua_reference
    src/perspective_camera.cpp
    src/reference_frame.cpp
    src/reference_manifest.cpp
    src/reference_output.cpp
    src/reference_ray.cpp
    src/reference_renderer.cpp
    src/reference_scene.cpp
    src/reference_serialization.cpp
    src/solar_kerr_ray_tracer.cpp)
target_compile_features(gargantua_reference PUBLIC cxx_std_17)
target_include_directories(
    gargantua_reference
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
    gargantua_reference
    PRIVATE Solar::Relativity)
target_compile_definitions(
    gargantua_reference
    PRIVATE
        GARGANTUA_BUILD_GIT_COMMIT="${GARGANTUA_GIT_COMMIT}"
        GARGANTUA_BUILD_DIRTY=${GARGANTUA_BUILD_DIRTY}
        GARGANTUA_SOLAR_GIT_COMMIT="${GARGANTUA_SOLAR_COMMIT}")
gargantua_enable_warnings(gargantua_reference)

add_library(
    gargantua_reference_cli_options
    cli/reference_render_options.cpp)
target_compile_features(
    gargantua_reference_cli_options
    PUBLIC cxx_std_17)
target_include_directories(
    gargantua_reference_cli_options
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cli")
target_link_libraries(
    gargantua_reference_cli_options
    PUBLIC gargantua_reference)
gargantua_enable_warnings(gargantua_reference_cli_options)

add_executable(
    gargantua-render-reference
    cli/reference_render_main.cpp)
target_compile_features(
    gargantua-render-reference
    PRIVATE cxx_std_17)
target_link_libraries(
    gargantua-render-reference
    PRIVATE gargantua_reference_cli_options)
gargantua_enable_warnings(gargantua-render-reference)

add_executable(gargantua-probe src/probe_main.cpp)
target_compile_features(gargantua-probe PRIVATE cxx_std_17)
target_compile_definitions(
    gargantua-probe
    PRIVATE
        GARGANTUA_REQUIRED_SOLAR_VERSION="${GARGANTUA_SOLAR_VERSION}"
        GARGANTUA_REQUIRED_SOLAR_CONTRACT="${GARGANTUA_SOLAR_PHYSICS_CONTRACT}")
target_link_libraries(gargantua-probe PRIVATE Solar::Relativity)
gargantua_enable_warnings(gargantua-probe)

add_test(NAME gargantua.solar_probe COMMAND gargantua-probe)
set_tests_properties(
    gargantua.solar_probe
    PROPERTIES
        PASS_REGULAR_EXPRESSION
            "\"physics_contract\":\"${GARGANTUA_SOLAR_PHYSICS_CONTRACT}\"")

if(GARGANTUA_BUILD_TESTS)
    add_executable(test-scene-camera tests/test_scene_camera.cpp)
    target_compile_features(test-scene-camera PRIVATE cxx_std_17)
    target_link_libraries(test-scene-camera PRIVATE gargantua_reference)
    gargantua_enable_warnings(test-scene-camera)
    add_test(NAME gargantua.scene_camera COMMAND test-scene-camera)

    add_executable(
        test-solar-kerr-ray-tracer
        tests/test_solar_kerr_ray_tracer.cpp)
    target_compile_features(
        test-solar-kerr-ray-tracer
        PRIVATE cxx_std_17)
    target_link_libraries(
        test-solar-kerr-ray-tracer
        PRIVATE gargantua_reference)
    gargantua_enable_warnings(test-solar-kerr-ray-tracer)
    add_test(
        NAME gargantua.solar_kerr_ray_tracer
        COMMAND test-solar-kerr-ray-tracer)

    add_executable(
        test-reference-renderer
        tests/test_reference_renderer.cpp)
    target_compile_features(
        test-reference-renderer
        PRIVATE cxx_std_17)
    target_link_libraries(
        test-reference-renderer
        PRIVATE gargantua_reference)
    gargantua_enable_warnings(test-reference-renderer)
    add_test(
        NAME gargantua.reference_renderer
        COMMAND test-reference-renderer)

    add_executable(
        test-reference-output
        tests/test_reference_output.cpp)
    target_compile_features(
        test-reference-output
        PRIVATE cxx_std_17)
    target_link_libraries(
        test-reference-output
        PRIVATE gargantua_reference)
    gargantua_enable_warnings(test-reference-output)
    add_test(
        NAME gargantua.reference_output
        COMMAND test-reference-output)

    add_executable(
        test-reference-render-options
        tests/test_reference_render_options.cpp)
    target_compile_features(
        test-reference-render-options
        PRIVATE cxx_std_17)
    target_include_directories(
        test-reference-render-options
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cli")
    target_link_libraries(
        test-reference-render-options
        PRIVATE gargantua_reference_cli_options)
    gargantua_enable_warnings(test-reference-render-options)
    add_test(
        NAME gargantua.reference_render_options
        COMMAND test-reference-render-options)

    add_test(
        NAME gargantua.reference_render_cli
        COMMAND
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_reference_render_cli.sh"
            "$<TARGET_FILE:gargantua-render-reference>")
endif()
