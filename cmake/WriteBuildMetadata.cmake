if(NOT DEFINED GARGANTUA_SOURCE_DIR)
    message(FATAL_ERROR "GARGANTUA_SOURCE_DIR is required")
endif()
if(NOT DEFINED GARGANTUA_METADATA_OUTPUT)
    message(FATAL_ERROR "GARGANTUA_METADATA_OUTPUT is required")
endif()

execute_process(
    COMMAND git -C "${GARGANTUA_SOURCE_DIR}" rev-parse HEAD
    RESULT_VARIABLE gargantua_git_result
    OUTPUT_VARIABLE gargantua_git_commit
    ERROR_VARIABLE gargantua_git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(LENGTH "${gargantua_git_commit}" gargantua_commit_length)
if(
    NOT gargantua_git_result EQUAL 0 OR
    NOT gargantua_commit_length EQUAL 40 OR
    NOT gargantua_git_commit MATCHES "^[0-9a-f]+$")
    message(
        FATAL_ERROR
        "Gargantua source has no valid Git commit: ${gargantua_git_error}")
endif()

execute_process(
    COMMAND git -C "${GARGANTUA_SOURCE_DIR}" status --porcelain
    RESULT_VARIABLE gargantua_status_result
    OUTPUT_VARIABLE gargantua_git_status
    ERROR_VARIABLE gargantua_status_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT gargantua_status_result EQUAL 0)
    message(
        FATAL_ERROR
        "Cannot inspect Gargantua worktree: ${gargantua_status_error}")
endif()
if(gargantua_git_status STREQUAL "")
    set(gargantua_build_dirty 0)
else()
    set(gargantua_build_dirty 1)
endif()

get_filename_component(
    gargantua_metadata_directory
    "${GARGANTUA_METADATA_OUTPUT}"
    DIRECTORY)
file(MAKE_DIRECTORY "${gargantua_metadata_directory}")
set(gargantua_metadata_temporary "${GARGANTUA_METADATA_OUTPUT}.tmp")
file(
    WRITE
    "${gargantua_metadata_temporary}"
    "#pragma once\n"
    "\n"
    "#define GARGANTUA_BUILD_GIT_COMMIT \"${gargantua_git_commit}\"\n"
    "#define GARGANTUA_BUILD_DIRTY ${gargantua_build_dirty}\n")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${gargantua_metadata_temporary}"
        "${GARGANTUA_METADATA_OUTPUT}"
    RESULT_VARIABLE gargantua_copy_result)
file(REMOVE "${gargantua_metadata_temporary}")
if(NOT gargantua_copy_result EQUAL 0)
    message(FATAL_ERROR "Cannot update Gargantua build metadata")
endif()
