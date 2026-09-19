/* Copyrights Uri Cohen uri.l.cohen@gmail.com 2026 */
/* clang-format off */

%{

#include <noise_std_include.h>
#include <DocLexerExtra.h>
using noise::DocLexerExtra;
using std::string;

#include <DocParser.tab.hh>
#define YY_DECL noise::DocParser::symbol_type yylex(yyscan_t yyscanner)

/* Macro to automatically advance columns and lines on every match */
#define YY_USER_ACTION \
    yyextra->loc.step(); \
    yyextra->loc.columns(yyleng);

#define yyterminate() return noise::DocParser::make_END_OF_FILE(yyextra->loc)

%}

%option case-insensitive
%option batch
%option 8bit
%option stack
%option reentrant
%option noyywrap
%option extra-type="DocLexerExtra*"
%option prefix="doc_"

/*
 *********************************************************
 * When adding a state don't forget updating DOC_PUSH_STATES
 * in src/DocLexerExtra.h
 *
 * EXPECT_PARAMS_STATE/EXPECT_ARGS_STATE are "peeking" states: pushed right
 * after a MACRO token, before we know whether a "<...>"/"(...)" actually
 * follows. They behave exactly like INITIAL (a bare macro call with no
 * params/args must not disturb ordinary surrounding document text) except
 * for recognizing their own opening delimiter, which pushes the
 * corresponding *_VALUES_STATE where ParamList/ArgList content (attrs,
 * names, ',', nested calls, raw text) is actually scanned.
 *********************************************************
 */
%x EXPECT_PARAMS_STATE
%x EXPECT_ARGS_STATE
%x PARAM_VALUES_STATE
%x ARG_VALUES_STATE
%x Q_STRING_STATE
%x QQ_STRING_STATE

ID  [a-zA-Z][a-zA-Z0-9_]*

%%

<*><<EOF>> { yyterminate(); }

<Q_STRING_STATE>([^'\n]|\\')* {
    return noise::DocParser::make_NO_Q_LINE(yytext, yyextra->loc);
}
<QQ_STRING_STATE>([^\"\n]|\\\")* {
    return noise::DocParser::make_NO_QQ_LINE(yytext, yyextra->loc);
}
<Q_STRING_STATE>\' {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type('\'', yyextra->loc);
}
<QQ_STRING_STATE>\" {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type('"', yyextra->loc);
}

 /* -- rules shared by INITIAL and the two "peeking" states -- */

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>(param|arg)\[[0-9]+\] {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>#(params|args|line|col) {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

 /* Same lookups, but a value's raw content is a NO_COMMA_TEXT, not a TEXT -
  * balanced_text has no alternative accepting a bare TEXT token. */
<PARAM_VALUES_STATE,ARG_VALUES_STATE>(param|arg)\[[0-9]+\] {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>#(params|args|line|col) {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>{ID} {
    if (yyextra->is_macro(yytext)) {
        return noise::DocParser::make_MACRO(yytext, yyextra->loc);
    }
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\n {
    yyextra->loc.lines(1);
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>[\'\"] {
    if (yytext[0] == '\'') {
        doc_push_Q_STRING_STATE(yyscanner);
    } else {
        doc_push_QQ_STRING_STATE(yyscanner);
    }
    return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
}

 /* -- the only thing that distinguishes the "peeking" states from INITIAL -- */

 /* These return the delimiter as a plain token WITHOUT pushing the content
  * state themselves - the push happens from the grammar (opt_params'/
  * opt_args' own mid-rule action, right after the delimiter is actually
  * shifted there). This matters because a '(' seen while still "peeking"
  * in EXPECT_PARAMS_STATE (no '<...>' present) must survive, unconsumed,
  * as the very token opt_args later shifts - if this rule pushed
  * ARG_VALUES_STATE itself, that push would race the pending-lookahead
  * reuse and scan the wrong content with the wrong state. */
<EXPECT_PARAMS_STATE>\< {
    return noise::DocParser::symbol_type('<', yyextra->loc);
}
<EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\( {
    return noise::DocParser::symbol_type('(', yyextra->loc);
}

 /* '>' / ')' / ',' are ambiguous while peeking: if this whole `expandable`
  * is itself nested inside an enclosing param/arg value (e.g. a bare macro
  * reference used as a positional arg, with no params/args of its own,
  * immediately followed by the *enclosing* list's ',' or closing
  * delimiter), these characters must stay distinct tokens so the outer
  * param_list/arg_list grammar can still consume them. At document top
  * level they're just literal text. doc_top_state() reports the state we
  * pushed EXPECT_PARAMS_STATE/EXPECT_ARGS_STATE from, i.e. the enclosing
  * context. */
<EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>[>),] {
    int parent = doc_top_state(yyscanner);
    if (parent == PARAM_VALUES_STATE || parent == ARG_VALUES_STATE) {
        return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
    }
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

<INITIAL>[^a-zA-Z\n'\"#]+ {
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}
<EXPECT_PARAMS_STATE>[^a-zA-Z\n'\"#<(>),]+ {
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}
<EXPECT_ARGS_STATE>[^a-zA-Z\n'\"#(>),]+ {
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

 /* A lone '#' not starting one of the four builtins is just literal text. */
<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE># {
    return noise::DocParser::make_TEXT(yytext, yyextra->loc);
}

 /* -- ParamList/ArgList content: only reached once '<' or '(' was actually seen -- */

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{
required { return noise::DocParser::make_REQUIRED(yyextra->loc); }
trim     { return noise::DocParser::make_TRIM(yyextra->loc); }
quoted   { return noise::DocParser::make_QUOTED(yyextra->loc); }
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{ID}/[ \t]*= {
    return noise::DocParser::make_ID(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{ID} {
    if (yyextra->is_macro(yytext)) {
        return noise::DocParser::make_MACRO(yytext, yyextra->loc);
    }
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>[ \t]+ { /* insignificant around attrs/name/punctuation */ }

<PARAM_VALUES_STATE,ARG_VALUES_STATE>[,=] {
    return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>[\'\"] {
    if (yytext[0] == '\'') {
        doc_push_Q_STRING_STATE(yyscanner);
    } else {
        doc_push_QQ_STRING_STATE(yyscanner);
    }
    return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
}

<PARAM_VALUES_STATE>> {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type('>', yyextra->loc);
}
<ARG_VALUES_STATE>\) {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type(')', yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>([^,'\"=<>()#\\\n \t]|\\.)+ {
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

 /* A lone '#' not starting one of the four builtins above is just literal
  * content (falls through here since the specific rule above always wins
  * on length when it does match). */
<PARAM_VALUES_STATE,ARG_VALUES_STATE># {
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

%%

#define X(S) void doc_push_##S(yyscan_t s) { yy_push_state(S, s); }
DOC_PUSH_STATES;
#undef X

void doc_pop_state(yyscan_t yyscanner)
{
    yy_pop_state(yyscanner);
}

int doc_top_state(yyscan_t yyscanner)
{
    return yy_top_state(yyscanner);
}
