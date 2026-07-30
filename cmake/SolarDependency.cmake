include(FetchContent)
include("${CMAKE_CURRENT_LIST_DIR}/solar-lock.cmake")

set(SOLAR_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(
    GARGANTUA_SOLAR_SOURCE_DIR
    ""
    CACHE PATH
    "Optional local Solar checkout matching the locked commit")

if(GARGANTUA_SOLAR_SOURCE_DIR)
    execute_process(
        COMMAND git -C "${GARGANTUA_SOLAR_SOURCE_DIR}" rev-parse HEAD
        RESULT_VARIABLE solar_git_result
        OUTPUT_VARIABLE solar_git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(
        NOT solar_git_result EQUAL 0 OR
        NOT solar_git_commit STREQUAL GARGANTUA_SOLAR_COMMIT)
        message(
            FATAL_ERROR
            "Local Solar checkout does not match cmake/solar-lock.cmake")
    endif()
    add_subdirectory(
        "${GARGANTUA_SOLAR_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/_deps/solar-build"
        EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        solar
        GIT_REPOSITORY "${GARGANTUA_SOLAR_REPOSITORY}"
        GIT_TAG "${GARGANTUA_SOLAR_COMMIT}"
        GIT_SHALLOW FALSE)
    FetchContent_MakeAvailable(solar)
endif()
