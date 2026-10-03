// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// this file is the Bison/Flex shared data with DocBuilder.
// Non inline methods are implemented in DocBuilder.cpp

#pragma once

#include <noise_std_include.h>

#include <Binding.h>
#include <doc_location.hh>

typedef void* yyscan_t;

// these are all DocLexer %x States
// unfortunately flex does not allow macros in in that section so the list
// is duplicated
#define DOC_PUSH_STATES         \
    X(EXPECT_PARAMS_STATE)      \
    X(EXPECT_ARGS_STATE)        \
    X(PARAM_VALUES_STATE)       \
    X(ARG_VALUES_STATE)         \
    X(LP_STATE)                 \
    X(LA_STATE)                 \
    X(Q_STRING_STATE)           \
    X(QQ_STRING_STATE)

#define X(S) void doc_push_##S(yyscan_t s);
DOC_PUSH_STATES;
#undef X

void doc_pop_state(yyscan_t yyscanner);
int doc_top_state(yyscan_t yyscanner);

namespace noise {

class NoiseFlow;

struct DocLexerExtra {
    DocLexerExtra(NoiseFlow* o, const std::string& stream_id, std::ostream* out);

    NoiseFlow* owner;
    std::string stream_id;
    std::ostream* out;
    uint32_t out_line;
    uint32_t out_col;

    // ParamList/ArgList item scanning state, see IN_VALUE in DocLexer.l
    bool item_head = false;
    bool after_id = false;

    // Name of the macro whose ParamList/ArgList is currently being scanned
    // (set by expandable's own mid-rule action right after the MACRO token
    // is shifted). Lets the lexer recognize a bare "name" entry - with no
    // "=value" - as a flag for a BOOL-attributed formal of *this specific*
    // macro (see the {ID} rule in DocLexer.l), without ever seeing one
    // nested inside another call's value: balanced_text never re-enters
    // expandable, so this never needs to be a stack.
    std::string pending_macro_name;

    bool is_macro(const std::string& id) const;
    std::optional<std::string> context_value(const std::string& id) const;
    bool is_bool_formal(const std::string& id) const;
    // records an unknown $name at the current location (see NoiseFlow)
    void unknown_name(const std::string& name) const;

    void emit(const std::string& text);

    location loc;
};

}  // namespace noise
