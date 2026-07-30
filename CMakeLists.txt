cmake_minimum_required(VERSION 3.20)

project(GargantuaStudio VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(cmake/SolarDependency.cmake)

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
