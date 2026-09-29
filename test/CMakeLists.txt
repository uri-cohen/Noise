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

add_cli_test(basic)
add_cli_test(args)
add_cli_test(attrs)
add_cli_test(peek)
add_cli_test(balanced)
add_cli_test(escape)
add_cli_test(repeat)
add_cli_test(select)
add_cli_test(foreach)
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
add_cli_test(global_params -D who=world -D level=3 --define=flag -D who=everyone)
