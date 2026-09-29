# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# Runs the noise binary against a def/doc pair (as run_cli_test.cmake, but
# its randomized output isn't diffed) and fails if it takes longer than its
# time budget. Invoked by ctest via CMakeLists.txt's add_cli_perf_test();
# expects NOISE_EXE, DEF_FILE, IN_FILE, BUDGET_MS, SCALE, TEST_NAME and
# (optionally) EXTRA_ARGS - '|'-separated noise args passed before --def.
#
# The budget is BUDGET_MS x NOISE_PERF_SCALE: the NOISE_PERF_SCALE environment
# variable (at ctest time) if set, else SCALE (the NOISE_PERF_SCALE CMake
# cache variable).

string(REPLACE "|" ";" extra_args "${EXTRA_ARGS}")

if(DEFINED ENV{NOISE_PERF_SCALE})
    set(SCALE "$ENV{NOISE_PERF_SCALE}")
endif()
if(NOT SCALE MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR
        "NOISE_PERF_SCALE must be a positive whole number, got '${SCALE}'")
endif()
math(EXPR budget_ms "${BUDGET_MS} * ${SCALE}")

string(TIMESTAMP start_us "%s%f")
execute_process(
    COMMAND ${NOISE_EXE} ${extra_args} --def ${DEF_FILE} ${IN_FILE}
    OUTPUT_QUIET
    ERROR_VARIABLE stderr_output
    RESULT_VARIABLE noise_result
)
string(TIMESTAMP end_us "%s%f")
math(EXPR elapsed_ms "(${end_us} - ${start_us}) / 1000")

if(NOT noise_result EQUAL 0)
    message(FATAL_ERROR
        "noise exited with status ${noise_result}\n${stderr_output}")
endif()

message(STATUS "${TEST_NAME}: ${elapsed_ms} ms (budget ${budget_ms} ms)")

if(elapsed_ms GREATER budget_ms)
    # smallest whole scale that would have passed, plus some headroom
    math(EXPR needed "(${elapsed_ms} + ${BUDGET_MS} - 1) / ${BUDGET_MS} + 1")
    message(FATAL_ERROR
        "noise performance test '${TEST_NAME}' took ${elapsed_ms} ms - over "
        "its budget of ${budget_ms} ms (base ${BUDGET_MS} ms x "
        "NOISE_PERF_SCALE ${SCALE}).\n"
        "If this host is just slower (or loaded, or this is a sanitizer/"
        "coverage build) rather than noise having regressed, scale the "
        "budget up, e.g. by ${needed}:\n"
        "  for one run:      NOISE_PERF_SCALE=${needed} ctest --test-dir <build-dir>\n"
        "  for this build:   cmake -DNOISE_PERF_SCALE=${needed} <build-dir>\n"
        "or skip the performance tests altogether:\n"
        "  ctest --test-dir <build-dir> -LE perf\n")
endif()
