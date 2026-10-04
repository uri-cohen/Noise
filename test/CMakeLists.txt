# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# pytest/SWIG-based tests are not runnable yet: SWIG bindings (noise_api_py)
# are commented out in src/CMakeLists.txt, so there is no noise_api module
# for these to import. Left here for when that lands.
# add_test(NAME post_build_pytest
#          COMMAND pytest -m pb ${CMAKE_SOURCE_DIR}
#          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
# )
#
# add_test(NAME pytest
#          COMMAND pytest ${CMAKE_SOURCE_DIR}
#          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
# )
#
# add_custom_target(run_post_build_pytest ALL
#     COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
#     DEPENDS noise_api_py
# )

# ── CLI smoke tests ───────────────────────────────────────────────────────
# End-to-end regression checks that don't depend on the (not yet built)
# SWIG/Python bindings: run the noise binary against a def/doc pair and
# diff its output against the recorded .ex file.

# Any further args are extra noise command line args (passed before --def).
# They are '|'-joined here since a ';' list would be split by add_test.
function(add_cli_test test_name)
    string(JOIN "|" extra_args ${ARGN})
    add_test(
        NAME cli_${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOISE_EXE=$<TARGET_FILE:noise>
            "-DEXTRA_ARGS=${extra_args}"
            -DDEF_FILE=${CMAKE_CURRENT_SOURCE_DIR}/def/${test_name}.def
            -DIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/in/${test_name}.in
            -DEXPECTED_FILE=${CMAKE_CURRENT_SOURCE_DIR}/expected/${test_name}.ex
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_test.cmake
    )
endfunction()

# Like add_cli_test, but noise must fail cleanly (exit status 1 - a crash
# doesn't count) and its stderr (the error) is diffed against the .ex file.
function(add_cli_error_test test_name)
    string(JOIN "|" extra_args ${ARGN})
    add_test(
        NAME cli_${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOISE_EXE=$<TARGET_FILE:noise>
            "-DEXTRA_ARGS=${extra_args}"
            -DEXPECT_ERROR=1
            -DDEF_FILE=${CMAKE_CURRENT_SOURCE_DIR}/def/${test_name}.def
            -DIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/in/${test_name}.in
            -DEXPECTED_FILE=${CMAKE_CURRENT_SOURCE_DIR}/expected/${test_name}.ex
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_test.cmake
    )
endfunction()

# Like add_cli_test, but its stderr - the warnings - is diffed too, against
# expected/<test_name>.err (and noise must exit with status 0).
function(add_cli_warning_test test_name)
    string(JOIN "|" extra_args ${ARGN})
    add_test(
        NAME cli_${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOISE_EXE=$<TARGET_FILE:noise>
            "-DEXTRA_ARGS=${extra_args}"
            -DDEF_FILE=${CMAKE_CURRENT_SOURCE_DIR}/def/${test_name}.def
            -DIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/in/${test_name}.in
            -DEXPECTED_FILE=${CMAKE_CURRENT_SOURCE_DIR}/expected/${test_name}.ex
            -DWARN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/expected/${test_name}.err
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_test.cmake
    )
endfunction()

# Performance regression check: runs a def/doc pair like add_cli_test (but
# without diffing its randomized output) and fails if it takes longer than
# budget_ms x NOISE_PERF_SCALE. The base budgets leave a wide margin (~10x)
# over a current machine; a slower host scales them up (see
# run_perf_test.cmake's failure message). Labeled "perf": ctest -LE perf
# skips them, ctest -L perf runs only them.
set(NOISE_PERF_SCALE 1 CACHE STRING
    "Whole number multiplier of the performance tests' time budgets (a slower host needs more)")

function(add_cli_perf_test test_name budget_ms)
    string(JOIN "|" extra_args ${ARGN})
    add_test(
        NAME perf_${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOISE_EXE=$<TARGET_FILE:noise>
            "-DEXTRA_ARGS=${extra_args}"
            -DTEST_NAME=${test_name}
            -DBUDGET_MS=${budget_ms}
            -DSCALE=${NOISE_PERF_SCALE}
            -DDEF_FILE=${CMAKE_CURRENT_SOURCE_DIR}/def/${test_name}.def
            -DIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/in/${test_name}.in
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_perf_test.cmake
    )
    set_tests_properties(perf_${test_name} PROPERTIES LABELS perf)
endfunction()

add_cli_test(basic)
add_cli_test(args)
add_cli_test(attrs)
add_cli_test(peek)
add_cli_test(balanced)
add_cli_test(escape)
add_cli_test(repeat)
add_cli_test(select)
add_cli_error_test(select_text_twice)
add_cli_test(foreach)
add_cli_error_test(foreach_items_twice)
add_cli_test(uniform_dist)
add_cli_test(exp_dist)
add_cli_test(normal_dist)
add_cli_test(binomial_dist)
add_cli_test(poisson_dist)
add_cli_test(vars_basic)
add_cli_test(vars_types)
add_cli_test(vars_macro)
add_cli_test(vars_string)
add_cli_test(vars_fallback)
add_cli_test(vars_smtlib)
add_cli_test(config_params -D NOISE_REAL_RANGE=0.001)
add_cli_test(depth_limit -D NOISE_MAX_DEPTH=3)
add_cli_error_test(depth_self_recursion)
add_cli_test(expansion_limit -D NOISE_MAX_EXPANSIONS=4)
add_cli_error_test(expansion_limit_error -D NOISE_MAX_EXPANSIONS=4)
add_cli_test(substr)
add_cli_error_test(substr_range)
add_cli_test(vars_bitslice)
add_cli_error_test(vars_bitslice_range)
add_cli_error_test(vars_bitslice_type)
add_cli_warning_test(plain_words -D b=5 -D f=9)
add_cli_warning_test(sigil)
set_tests_properties(cli_sigil PROPERTIES
    ENVIRONMENT "NOISE_SIGIL_GREETING=hello from the environment;NOISE_SIGIL_NUM=42")
add_cli_warning_test(sigil_unknown_file -D NOISE_MAX_UNKNOWN_WARNING=2)
add_cli_test(macro_id)
add_cli_test(concat)
add_cli_warning_test(ite)
add_cli_error_test(ite_missing_cond)
add_cli_test(range)
add_cli_error_test(range_too_long)
add_cli_test(long_text)
add_cli_test(expr)
add_cli_error_test(expr_error)
add_cli_test(minmax)
add_cli_test(assign_modes -D d=42)
add_cli_test(global_params -D who=world -D level=3 --define=flag -D who=everyone)

# ~0.6s on a current machine (unoptimized build)
add_cli_perf_test(vars_load 6000)
