if(NOT DEFINED SOLAR_SOURCE_DIR OR NOT DEFINED LOCK_OUTPUT)
    message(FATAL_ERROR "SOLAR_SOURCE_DIR and LOCK_OUTPUT are required")
endif()

execute_process(
    COMMAND git -C "${SOLAR_SOURCE_DIR}" rev-parse HEAD
    RESULT_VARIABLE solar_git_result
    OUTPUT_VARIABLE solar_git_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
    COMMAND git -C "${SOLAR_SOURCE_DIR}" status --porcelain
    RESULT_VARIABLE solar_status_result
    OUTPUT_VARIABLE solar_status
    OUTPUT_STRIP_TRAILING_WHITESPACE)

string(LENGTH "${solar_git_commit}" solar_commit_length)
if(
    NOT solar_git_result EQUAL 0 OR
    NOT solar_status_result EQUAL 0 OR
    NOT solar_commit_length EQUAL 40 OR
    NOT solar_git_commit MATCHES "^[0-9a-f]+$" OR
    NOT solar_status STREQUAL "")
    message(FATAL_ERROR "Solar source must be a clean, valid Git commit")
endif()

set(solar_version_header "${SOLAR_SOURCE_DIR}/include/solar/version.h")
if(NOT EXISTS "${solar_version_header}")
    message(FATAL_ERROR "Solar source has no public version header")
endif()

file(READ "${solar_version_header}" solar_version_contents)
string(
    REGEX MATCH
    "version\\{\"([^\"]+)\"\\}"
    solar_version_match
    "${solar_version_contents}")
set(solar_version "${CMAKE_MATCH_1}")
string(
    REGEX MATCH
    "physics_contract\\{\"([^\"]+)\"\\}"
    solar_contract_match
    "${solar_version_contents}")
set(solar_physics_contract "${CMAKE_MATCH_1}")
if(NOT solar_version_match OR NOT solar_contract_match)
    message(FATAL_ERROR "Solar public version contract cannot be parsed")
endif()

file(
    WRITE "${LOCK_OUTPUT}"
    "set(GARGANTUA_SOLAR_REPOSITORY \"https://github.com/TT1nKer/solar.git\")\n"
    "set(GARGANTUA_SOLAR_COMMIT \"${solar_git_commit}\")\n"
    "set(GARGANTUA_SOLAR_VERSION \"${solar_version}\")\n"
    "set(GARGANTUA_SOLAR_PHYSICS_CONTRACT \"${solar_physics_contract}\")\n")
