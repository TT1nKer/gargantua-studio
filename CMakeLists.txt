cmake_minimum_required(VERSION 3.20)

project(GargantuaStudio VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(cmake/SolarDependency.cmake)

option(GARGANTUA_BUILD_TESTS "Build Gargantua tests" ON)

add_library(
    gargantua_reference
    src/perspective_camera.cpp
    src/reference_frame.cpp
    src/reference_ray.cpp
    src/reference_renderer.cpp
    src/reference_scene.cpp
    src/solar_kerr_ray_tracer.cpp)
target_compile_features(gargantua_reference PUBLIC cxx_std_17)
target_include_directories(
    gargantua_reference
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
    gargantua_reference
    PRIVATE Solar::Relativity)

add_executable(gargantua-probe src/probe_main.cpp)
target_compile_features(gargantua-probe PRIVATE cxx_std_17)
target_compile_definitions(
    gargantua-probe
    PRIVATE
        GARGANTUA_REQUIRED_SOLAR_VERSION="${GARGANTUA_SOLAR_VERSION}"
        GARGANTUA_REQUIRED_SOLAR_CONTRACT="${GARGANTUA_SOLAR_PHYSICS_CONTRACT}")
target_link_libraries(gargantua-probe PRIVATE Solar::Relativity)

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
    add_test(
        NAME gargantua.reference_renderer
        COMMAND test-reference-renderer)
endif()
