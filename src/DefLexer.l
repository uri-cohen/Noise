/* Copyrights Uri Cohen uri.l.cohen@gmail.com 2026 */
/* clang-format off */

%{

#include <DefLexerExtra.h>
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
%x INCLUDE_STATE

%%

<*>{
[ \t]+		{ yyextra->loc.step(); }
\n		    { yyextra->loc.lines(1); yyextra->loc.step(); }
<<EOF>>     { yyterminate(); }
}

<INITIAL>#.*            {}
<PARAM_LIST_STATE>#.*   {}
<ARG_LIST_STATE>#.*     {}

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

<Q_STRING_STATE>([^'\n]|\\')* {
    return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
}
<QQ_STRING_STATE>([^\"\n]|\\\")* {
    return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
}
<INITIAL>\<\< {
    return noise::DefParser::make_LEFT_SHIFT(yyextra->loc);
}
<HERE_STRING_STATE>.* {
    if (string(yytext) == yyextra->delimiter) {
        return noise::DefParser::make_DELIMITER(yytext, yyextra->loc);
    } else {
        return noise::DefParser::make_ANY_STRING_LINE(yytext, yyextra->loc);
    }
}

<INCLUDE_STATE>[^>\n]+ {
    return noise::DefParser::make_FILE_NAME(yytext, yyextra->loc);
}

<PARAM_LIST_STATE,ARG_LIST_STATE>{
required { return noise::DefParser::make_REQUIRED(yyextra->loc); }
trim     { return noise::DefParser::make_TRIM(yyextra->loc); }
quoted   { return noise::DefParser::make_QUOTED(yyextra->loc); }
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
