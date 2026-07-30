if(NOT DEFINED GARGANTUA_METADATA_SCRIPT)
    message(FATAL_ERROR "GARGANTUA_METADATA_SCRIPT is required")
endif()
if(NOT DEFINED GARGANTUA_TEST_ROOT)
    message(FATAL_ERROR "GARGANTUA_TEST_ROOT is required")
endif()

function(run_checked)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE command_result
        OUTPUT_VARIABLE command_output
        ERROR_VARIABLE command_error)
    if(NOT command_result EQUAL 0)
        message(
            FATAL_ERROR
            "Command failed: ${ARGN}\n${command_output}${command_error}")
    endif()
endfunction()

function(refresh_metadata repository output)
    run_checked(
        "${CMAKE_COMMAND}"
        "-DGARGANTUA_SOURCE_DIR=${repository}"
        "-DGARGANTUA_METADATA_OUTPUT=${output}"
        -P
        "${GARGANTUA_METADATA_SCRIPT}")
endfunction()

function(assert_header_contains header expected)
    file(READ "${header}" header_contents)
    string(FIND "${header_contents}" "${expected}" match_index)
    if(match_index EQUAL -1)
        message(
            FATAL_ERROR
            "Expected ${header} to contain: ${expected}\n${header_contents}")
    endif()
endfunction()

file(REMOVE_RECURSE "${GARGANTUA_TEST_ROOT}")
set(test_repository "${GARGANTUA_TEST_ROOT}/repository")
set(test_header "${GARGANTUA_TEST_ROOT}/generated/build_metadata.h")
file(MAKE_DIRECTORY "${test_repository}")

run_checked(git -C "${test_repository}" init --quiet)
run_checked(
    git -C "${test_repository}"
    config user.email "metadata-test@example.invalid")
run_checked(
    git -C "${test_repository}"
    config user.name "Gargantua Metadata Test")
file(WRITE "${test_repository}/tracked.txt" "first\n")
run_checked(git -C "${test_repository}" add tracked.txt)
run_checked(
    git -C "${test_repository}" commit --quiet -m "initial")
execute_process(
    COMMAND git -C "${test_repository}" rev-parse HEAD
    OUTPUT_VARIABLE first_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE)

refresh_metadata("${test_repository}" "${test_header}")
assert_header_contains(
    "${test_header}"
    "#define GARGANTUA_BUILD_GIT_COMMIT \"${first_commit}\"")
assert_header_contains(
    "${test_header}"
    "#define GARGANTUA_BUILD_DIRTY 0")

file(APPEND "${test_repository}/tracked.txt" "dirty\n")
refresh_metadata("${test_repository}" "${test_header}")
assert_header_contains(
    "${test_header}"
    "#define GARGANTUA_BUILD_DIRTY 1")

run_checked(git -C "${test_repository}" add tracked.txt)
run_checked(
    git -C "${test_repository}" commit --quiet -m "second")
execute_process(
    COMMAND git -C "${test_repository}" rev-parse HEAD
    OUTPUT_VARIABLE second_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(first_commit STREQUAL second_commit)
    message(FATAL_ERROR "Second commit did not advance HEAD")
endif()

refresh_metadata("${test_repository}" "${test_header}")
assert_header_contains(
    "${test_header}"
    "#define GARGANTUA_BUILD_GIT_COMMIT \"${second_commit}\"")
assert_header_contains(
    "${test_header}"
    "#define GARGANTUA_BUILD_DIRTY 0")
