# Runs the noise binary against a def/doc pair and diffs its stdout against
# the recorded .ex file. Invoked by ctest via CMakeLists.txt's
# add_cli_test(); expects NOISE_EXE, DEF_FILE, IN_FILE, EXPECTED_FILE and
# (optionally) EXTRA_ARGS - '|'-separated noise args passed before --def -
# and EXPECT_ERROR: noise must then exit with status 1 and its stderr is
# what's diffed.

string(REPLACE "|" ";" extra_args "${EXTRA_ARGS}")

execute_process(
    COMMAND ${NOISE_EXE} ${extra_args} --def ${DEF_FILE} ${IN_FILE}
    OUTPUT_VARIABLE stdout_output
    ERROR_VARIABLE stderr_output
    RESULT_VARIABLE noise_result
)

if(EXPECT_ERROR)
    if(NOT noise_result EQUAL 1)
        message(FATAL_ERROR
            "noise exited with status ${noise_result}, expected 1\n${stderr_output}")
    endif()
    set(actual_output "${stderr_output}")
else()
    if(NOT noise_result EQUAL 0)
        message(FATAL_ERROR "noise exited with status ${noise_result}")
    endif()
    set(actual_output "${stdout_output}")
endif()

file(READ ${EXPECTED_FILE} expected_output)

if(NOT actual_output STREQUAL expected_output)
    message(FATAL_ERROR
        "Output mismatch for ${IN_FILE}\n"
        "--- expected ---\n${expected_output}\n"
        "--- actual ---\n${actual_output}\n"
    )
endif()
