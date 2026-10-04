/* Copyrights Uri Cohen uri.l.cohen@gmail.com 2026 */
/* clang-format off */

%{

#include <DefLexerExtra.h>
#include <Utils.h>
using noise::DefLexerExtra;
using std::string;

#include <DefParser.tab.hh>
#define YY_DECL noise::DefParser::symbol_type yylex(yyscan_t yyscanner)

/* Macro to automatically advance columns and lines on every match */
#define YY_USER_ACTION \
    yyextra->loc.step(); \
    yyextra->loc.columns(yyleng);

#define yyterminate() return noise::DefParser::make_END_OF_FILE(yyextra->loc)

%}

%option case-insensitive
%option batch
%option 8bit
%option stack
%option reentrant
%option noyywrap
%option extra-type="DefLexerExtra*"
%option prefix="def_"

/*
 *********************************************************
 * When adding a state don't forget updating PUSH_STATES
 * in src/core/DefLexerExtra.h
 *********************************************************
 */
%x Q_STRING_STATE
%x QQ_STRING_STATE
%x HERE_STRING_STATE
%x PARAM_LIST_STATE
%x ARG_LIST_STATE
%x DEF_VALUE_STATE
%x INCLUDE_STATE
%x VARS_STATE
%x VAR_VALUE_STATE

%%

<INITIAL,PARAM_LIST_STATE,ARG_LIST_STATE,DEF_VALUE_STATE,INCLUDE_STATE,VARS_STATE,VAR_VALUE_STATE>{
[ \t]+		{ yyextra->loc.step(); }
\n		    { yyextra->loc.lines(1); yyextra->loc.step(); }
}
<*><<EOF>>  { yyterminate(); }

<INITIAL>#.*            {}
<PARAM_LIST_STATE>#.*   {}
<ARG_LIST_STATE>#.*     {}
<DEF_VALUE_STATE>#.*    {}
<VARS_STATE>#.*         {}
<VAR_VALUE_STATE>#.*    {}

<INITIAL>MACRO  {
    return noise::DefParser::make_MACRO(yyextra->loc);
}
<INITIAL>INCLUDE  {
    return noise::DefParser::make_INCLUDE(yyextra->loc);
}
<INITIAL>TEXT  {
    return noise::DefParser::make_TEXT(yyextra->loc);
}
<INITIAL>PARAMS  {
    return noise::DefParser::make_PARAMS(yyextra->loc);
}
<INITIAL>ARGS  {
    return noise::DefParser::make_ARGS(yyextra->loc);
}
<INITIAL>VARS  {
    return noise::DefParser::make_VARS(yyextra->loc);
}

    /* string states are verbatim: each token is one line including its '\n' */
<Q_STRING_STATE>([^'\n]|\\')+\n?|\n {
    if (yytext[yyleng-1] == '\n') yyextra->loc.lines(1);
    return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
}
<QQ_STRING_STATE>([^\"\n]|\\\")+\n?|\n {
    if (yytext[yyleng-1] == '\n') yyextra->loc.lines(1);
    return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
}
<INITIAL>\<\< {
    return noise::DefParser::make_LEFT_SHIFT(yyextra->loc);
}
<HERE_STRING_STATE>.+\n?|\n {
    string line(yytext);
    bool nl = line.ends_with('\n');
    if (nl) { line.pop_back(); yyextra->loc.lines(1); }
    if (line == yyextra->delimiter) {
        return noise::DefParser::make_DELIMITER(line, yyextra->loc);
    }
    return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
}

<INCLUDE_STATE>[^>\n]+ {
    return noise::DefParser::make_FILE_NAME(yytext, yyextra->loc);
}

<PARAM_LIST_STATE,ARG_LIST_STATE>{
required { return noise::DefParser::make_REQUIRED(yyextra->loc); }
keep_left_ws|klws { return noise::DefParser::make_KEEP_LEFT_WS(yyextra->loc); }
keep_right_ws|krws { return noise::DefParser::make_KEEP_RIGHT_WS(yyextra->loc); }
keep_enclosing_ws|kws { return noise::DefParser::make_KEEP_ENCLOSING_WS(yyextra->loc); }
quoted   { return noise::DefParser::make_QUOTED(yyextra->loc); }
bool     { return noise::DefParser::make_BOOL(yyextra->loc); }
":="     { return noise::DefParser::make_COLON_EQ(yyextra->loc); }
}

 /* A param/arg value needs no quoting when it has none of the characters
  * that give quoting a reason to exist: ';' ends the entry, '#' starts a
  * comment, and a quote char starts one of the quoted forms. Whitespace
  * around it is trimmed (there is no quote to mark exact boundaries), but
  * internal whitespace is kept, so this still allows a bare multi-word
  * value. Only reached once '=' is actually shifted (see param_list/
  * arg_list in DefParser.yy). */
<DEF_VALUE_STATE>[^;#'\"\n]+ {
    return noise::DefParser::make_BARE_STRING(noise::trim(string(yytext)), yyextra->loc);
}

  /* A VARS clause: declarations, ASSERT constraint expressions and SMTLIB
   * code. Type names and operator keywords are reserved in here. */
<VARS_STATE>{
bool     { return noise::DefParser::make_BOOL(yyextra->loc); }
int      { return noise::DefParser::make_INT_T(yyextra->loc); }
uint     { return noise::DefParser::make_UINT_T(yyextra->loc); }
real     { return noise::DefParser::make_REAL_T(yyextra->loc); }
bitvec   { return noise::DefParser::make_BITVEC_T(yyextra->loc); }
string   { return noise::DefParser::make_STRING_T(yyextra->loc); }
assert   { return noise::DefParser::make_ASSERT(yyextra->loc); }
smtlib   { return noise::DefParser::make_SMTLIB(yyextra->loc); }
and      { return noise::DefParser::make_AND(yyextra->loc); }
or       { return noise::DefParser::make_OR(yyextra->loc); }
xor      { return noise::DefParser::make_XOR(yyextra->loc); }
not      { return noise::DefParser::make_NOT(yyextra->loc); }
"<<"     { return noise::DefParser::make_LEFT_SHIFT(yyextra->loc); }
">>"     { return noise::DefParser::make_SHR(yyextra->loc); }
"<="     { return noise::DefParser::make_LE(yyextra->loc); }
">="     { return noise::DefParser::make_GE(yyextra->loc); }
"=="     { return noise::DefParser::make_EQ(yyextra->loc); }
"!="     { return noise::DefParser::make_NE(yyextra->loc); }
"->"     { return noise::DefParser::make_IMPLIES(yyextra->loc); }
true|false|0x[0-9a-f]+|[0-9]+(\.[0-9]+)?(e[-+]?[0-9]+)? {
    return noise::DefParser::make_LITERAL(yytext, yyextra->loc);
}
[[:alpha:]_][[:alnum:]_]* {
    return noise::DefParser::make_ID(yytext, yyextra->loc);
}
}

 /* A variable's fallback value - as DEF_VALUE_STATE, but a ',' also ends it
  * (it may be followed by a further declaration of the same type). */
<VAR_VALUE_STATE>[^;,#'\"\n]+ {
    return noise::DefParser::make_BARE_STRING(noise::trim(string(yytext)), yyextra->loc);
}

<INITIAL,PARAM_LIST_STATE,ARG_LIST_STATE>[[:alpha:]][[:alnum:]_]*   {
    return noise::DefParser::make_ID(yytext, yyextra->loc);
}

<*>. { return noise::DefParser::symbol_type(yytext[0], yyextra->loc); }

%%

#define X(S) void def_push_##S(yyscan_t s) { yy_push_state(S, s); }
PUSH_STATES;
#undef X

void def_pop_state(yyscan_t yyscanner)
{
    yy_pop_state(yyscanner);
}
