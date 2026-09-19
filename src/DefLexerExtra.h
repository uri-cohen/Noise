// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// this file is the Bison/Flex shared data with DefBuilder.
// Non inline methods are implemented in DefBuilder.cpp

#pragma once

#include <noise_std_include.h>

#include <Binding.h>
#include <def_location.hh>

typedef void* yyscan_t;

// these are all DefLexer %x States
// unfortunately flex does not allow macros in  in that section so the list
// is duplicated
#define PUSH_STATES      \
    X(Q_STRING_STATE)    \
    X(QQ_STRING_STATE)   \
    X(HERE_STRING_STATE) \
    X(PARAM_LIST_STATE)  \
    X(ARG_LIST_STATE)    \
    X(INCLUDE_STATE)

#define X(S) void def_push_##S(yyscan_t s);
PUSH_STATES;
#undef X

void def_pop_state(yyscan_t yyscanner);

namespace noise {

class NoiseFlow;
class Macro;

struct DefLexerExtra {
    // ctor body in DefBuilder.cpp
    DefLexerExtra(NoiseFlow* o, const std::string& file_name);
    virtual ~DefLexerExtra() {};

    NoiseFlow* owner;
    std::string file_name;
    std::string delimiter;
    std::filesystem::path including_dir;

    // accumulator for the macro currently being parsed: TEXT/PARAMS/ARGS
    // may appear in any order inside "MACRO name = { ... }", so the real
    // TextMacro is only constructed once the closing '}' is reached.
    std::string curr_name;
    std::string curr_text;
    bool curr_text_set;
    std::vector<Binding> curr_params;
    std::vector<Binding> curr_args;

    std::vector<Macro*> macro_list;
    location loc;
};

}  // namespace noise
