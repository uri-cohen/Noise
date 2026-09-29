# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# find_package(SWIG 4.4.0 REQUIRED)
# find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

# Z3 (VARS constraint solving). The Ubuntu libz3-dev package ships no CMake
# config, so it is located directly.
find_path(Z3_INCLUDE_DIR z3++.h REQUIRED)
find_library(Z3_LIBRARY z3 REQUIRED)

# include(UseSWIG)

# ── Bison/Flex ─────────────────────────────────────────────────────────────────

FLEX_TARGET(DefLexer DefLexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/DefLexer.yy.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/DefLexer.yy.hh
)

BISON_TARGET(DefParser DefParser.yy
    ${CMAKE_CURRENT_BINARY_DIR}/DefParser.tab.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/DefParser.tab.hh
)

ADD_FLEX_BISON_DEPENDENCY(DefLexer DefParser)

# DocLexer/DocParser: document-expansion grammar, replacing the hand-rolled
# Tokenizer.
FLEX_TARGET(DocLexer DocLexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/DocLexer.yy.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/DocLexer.yy.hh
)

BISON_TARGET(DocParser DocParser.yy
    ${CMAKE_CURRENT_BINARY_DIR}/DocParser.tab.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/DocParser.tab.hh
)

ADD_FLEX_BISON_DEPENDENCY(DocLexer DocParser)

# ── Force the Flex output to compile as C++ ───────────────────────────────────
#
# Flex emits C, but the file must #include parser.tab.hh (a C++ header).
# Telling CMake the language is CXX makes the compiler treat it as C++
# without renaming the file.

set_source_files_properties(
    ${FLEX_DefLexer_OUTPUTS}
    PROPERTIES LANGUAGE CXX
)
set_source_files_properties(
    ${FLEX_DocLexer_OUTPUTS}
    PROPERTIES LANGUAGE CXX
)

# ── C++ library (the code being wrapped) ──────────────────────────────────────

add_library(noise_core SHARED
    Binding.cpp
    Binding.h
    Cmd.cpp
    Cmd.h
    Context.cpp
    Context.h
    Logger.cpp
    Logger.h
    DefBuilder.cpp
    DefBuilder.h
    DocBuilder.cpp
    DocBuilder.h
    Exception.cpp
    Exception.h
    Macro.cpp
    Macro.h
    NoiseFlow.cpp
    NoiseFlow.h
    NoiseFlowExpand.cpp
    NumericMacro.cpp
    NumericMacro.h
    TextMacro.cpp
    TextMacro.h
    Utils.h
    VarSolver.cpp
    VarSolver.h
    Vars.cpp
    Vars.h
    noise_std_include.h

    # Generated sources:
    # ******************
    ${BISON_DefParser_OUTPUTS}
    ${BISON_DocParser_OUTPUTS}
    # ^ .cc and .hh — but Bison's header is fine here
    # because CMake knows not to compile .hh from BISON_..._OUTPUTS
    ${CMAKE_CURRENT_BINARY_DIR}/DefLexer.yy.cc
    ${CMAKE_CURRENT_BINARY_DIR}/DocLexer.yy.cc
    # ^ cannot use ${FLEX_DefLexer_OUTPUTS}/${FLEX_DocLexer_OUTPUTS} as
    # current cmake version attempts compiling .hh file - unlike BISON
    # (same is making it .h file)
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(noise_core PRIVATE
        BUILD_DEBUG=1
        PROJECT=Noise)
else()
    target_compile_definitions(noise_core PRIVATE
        BUILD_DEBUG=0
        PROJECT=Noise)
endif()

target_include_directories(noise_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${Z3_INCLUDE_DIR}
 )

target_link_libraries(noise_core PRIVATE ${Z3_LIBRARY})

# ── executable ─────────────────────────────────────────────────────────────────

add_executable(noise
    Main.cpp
)
target_link_libraries(noise PRIVATE
    noise_core)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(noise PRIVATE
        BUILD_DEBUG=1
        PROJECT=Noise)
else()
    target_compile_definitions(noise PRIVATE
        BUILD_DEBUG=0
        PROJECT=Noise)
endif()

target_include_directories(noise PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ── TAGS  ──────────────────────────────────────────────────────────────────────

if(CTAGS_PATH)

    add_custom_target(ctags ALL
        COMMAND ${CTAGS_PATH} ${CTAGS_FLAGS} -f ${CMAKE_SOURCE_DIR}/src/TAGS ${CMAKE_SOURCE_DIR}/src
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
        COMMENT "Running ctags on ${CMAKE_SOURCE_DIR}/src"
        VERBATIM
    )

endif()

# # ── SWIG wrapper ───────────────────────────────────────────────────────────────
# # Tell SWIG this is C++ (not plain C)
# set_property(SOURCE py/noise_api.i PROPERTY CPLUSPLUS ON)
# set_property(SOURCE py/noise_api.i PROPERTY
#     COMPILE_OPTIONS -builtin
#                     -I${CMAKE_CURRENT_SOURCE_DIR}/core
#                     -I${CMAKE_CURRENT_SOURCE_DIR}/utils
#                     -I${CMAKE_CURRENT_SOURCE_DIR}/py
# )

# # Optional: pass flags to SWIG
# # set_property(SOURCE swig/mylib.i PROPERTY
# #     COMPILE_OPTIONS -doxygen -Wall
# # )

# swig_add_library(noise_api_py
#     TYPE        MODULE           # Python extension module
#     LANGUAGE    python
#     SOURCES     py/noise_api.i
# )

# # Link the extension against the real C++ lib and Python
# target_link_libraries(noise_api_py PRIVATE
#     noise_core
#     Python3::Module
# )

# # Ensure SWIG can find our headers
# target_include_directories(noise_api_py PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/core
#     ${CMAKE_CURRENT_SOURCE_DIR}/utils
#     ${CMAKE_CURRENT_SOURCE_DIR}/macrolib
#     ${CMAKE_CURRENT_SOURCE_DIR}/py
# )

# # ── Install ────────────────────────────────────────────────────────────────────
# # Discover where Python expects site-packages
# execute_process(
#     COMMAND "${Python3_EXECUTABLE}" -c
#         "import sysconfig; print(sysconfig.get_path('platlib'))"
#     OUTPUT_VARIABLE Python3_SITEARCH
#     OUTPUT_STRIP_TRAILING_WHITESPACE
# )

# install(TARGETS noise_api_py
#     LIBRARY DESTINATION "${Python3_SITEARCH}"
# )

# # The generated .py stub must be installed alongside the .so
# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/noise_api_py.py"
#     DESTINATION "${Python3_SITEARCH}"
# )
