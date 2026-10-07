# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# Runs the noise binary against a def/doc pair and diffs its stdout against
# the recorded .ex file. Invoked by ctest via CMakeLists.txt's
# add_cli_test(); expects NOISE_EXE, DEF_FILE, IN_FILE, EXPECTED_FILE and
# (optionally) EXTRA_ARGS - '|'-separated noise args passed before --def -
# and EXPECT_ERROR: noise must then exit with status 1 and its stderr is
# what's diffed - or WARN_FILE: noise must exit with status 0, its stdout is
# diffed against EXPECTED_FILE and its stderr (warnings) against WARN_FILE.
# With the NOISE_KEEP_ACTUAL environment variable set (to anything but empty)
# noise's stdout and stderr are also kept, in ACTUAL_FILE.out and .err - e.g.
#   NOISE_KEEP_ACTUAL=1 ctest --test-dir build/main -R cli_repeat
#   diff test/expected/repeat.ex build/main/test/actual/repeat.out

string(REPLACE "|" ";" extra_args "${EXTRA_ARGS}")

execute_process(
    COMMAND ${NOISE_EXE} ${extra_args} --def ${DEF_FILE} ${IN_FILE}
    OUTPUT_VARIABLE stdout_output
    ERROR_VARIABLE stderr_output
    RESULT_VARIABLE noise_result
)

# written before any check, so a failing test's output is kept too
if(NOT "$ENV{NOISE_KEEP_ACTUAL}" STREQUAL "" AND ACTUAL_FILE)
    file(WRITE "${ACTUAL_FILE}.out" "${stdout_output}")
    file(WRITE "${ACTUAL_FILE}.err" "${stderr_output}")
endif()

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
    if(WARN_FILE)
        file(READ ${WARN_FILE} expected_warnings)
        if(NOT stderr_output STREQUAL expected_warnings)
            message(FATAL_ERROR
                "Warnings mismatch for ${IN_FILE}\n"
                "--- expected ---\n${expected_warnings}\n"
                "--- actual ---\n${stderr_output}\n"
            )
        endif()
    endif()
endif()

file(READ ${EXPECTED_FILE} expected_output)

if(NOT actual_output STREQUAL expected_output)
    message(FATAL_ERROR
        "Output mismatch for ${IN_FILE}\n"
        "--- expected ---\n${expected_output}\n"
        "--- actual ---\n${actual_output}\n"
    )
endif()
