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

/* Inside a ParamList/ArgList item: item_head is true until the first token
 * that belongs to the item's value (attrs and "name=" may precede it);
 * after_id is true right after a "name". */
#define IN_VALUE (yyextra->item_head = false, yyextra->after_id = false)

/* whitespace in front of an attr keyword / "name=": a separator at the head
 * of an item, otherwise ordinary value text */
#define WS_BEFORE_HEAD_TOKEN \
    do { \
        for (int i = 0; i < yyleng; ++i) if (yytext[i] == '\n') yyextra->loc.lines(1); \
        if (yyextra->item_head) { yyextra->loc.step(); break; } \
        IN_VALUE; \
        return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc); \
    } while (0)

/* Literal text token. The peek states (EXPECT_*) are entered right after a
 * MACRO token; when that macro sits inside a param/arg value (the state they
 * were pushed from), the grammar expects value tokens, not document ones. */
#define IN_LIST_CONTEXT \
    (YY_START != INITIAL && (doc_top_state(yyscanner) == PARAM_VALUES_STATE || \
                             doc_top_state(yyscanner) == ARG_VALUES_STATE))
#define TEXT_TOKEN(S) \
    (IN_LIST_CONTEXT ? noise::DocParser::make_NO_COMMA_TEXT((S), yyextra->loc) \
                     : noise::DocParser::make_TEXT((S), yyextra->loc))

#define yyterminate() return noise::DocParser::make_END_OF_FILE(yyextra->loc)

/* The variable trailing context rules (e.g. "[ \t\n]+/{ID}[ \t\n]*=") make
 * flex record states in yy_state_buf while matching a token: the start state,
 * then one per character read - which, for a token running to the end of the
 * text, includes the two end-of-buffer sentinels yy_scan_string appends. Flex
 * allocates it with a fixed YY_BUF_SIZE+2 entries (16386), so a longer token -
 * e.g. a 16K+ run of plain text - overran it and corrupted the heap. The whole
 * text is in memory, so no token is longer than it: size the buffer by the
 * text (+3 needed; +16 for margin). Evaluated where flex allocates it, inside
 * yylex, where yyextra is available. */
#undef YY_STATE_BUF_SIZE
#define YY_STATE_BUF_SIZE ((yyextra->text_size + 16) * sizeof(yy_state_type))

/* "$name" (or "$param[i]") in text being expanded: the name's value; an
 * unknown name expands to nothing, and is recorded for the end-of-run
 * warning. */
static noise::DocParser::symbol_type expand_name(DocLexerExtra* x, const char* name) {
    if (auto cv = x->context_value(name)) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, x->loc);
    }
    x->unknown_name(name);
    return noise::DocParser::make_CONTEXT_VALUE("", x->loc);
}

/* "$name" inside a param/arg value: its value if the name is known now;
 * otherwise kept as raw text ("$name"), to be resolved when the value itself
 * is expanded - an arg may well refer to a name only bound then (e.g. a
 * foreach loop variable). */
static noise::DocParser::symbol_type value_name(DocLexerExtra* x, const char* text) {
    if (auto cv = x->context_value(text + 1)) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, x->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(text, x->loc);
}

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
 * names, ',', raw text) is actually scanned.
 *
 * LP_STATE/LA_STATE capture a literal "(...)"/"<...>" group found inside a
 * param/arg VALUE that is not itself a macro call - balanced_text just needs
 * to find the matching close without misreading it as the end of the
 * enclosing list. They reuse item_head/after_id's current (already-false)
 * value, so no attrs/name lookup happens inside them: content is captured
 * verbatim, with nested (), <>, quotes and ',' still recognized so a further
 * nested group or the enclosing list's own comma/close is never swallowed.
 *********************************************************
 */
%x EXPECT_PARAMS_STATE
%x EXPECT_ARGS_STATE
%x PARAM_VALUES_STATE
%x ARG_VALUES_STATE
%x LP_STATE
%x LA_STATE
%x Q_STRING_STATE
%x QQ_STRING_STATE

ID  [a-zA-Z][a-zA-Z0-9_]*

%%

<*><<EOF>> { yyterminate(); }

<Q_STRING_STATE>[^'\n\\]+ {
    return noise::DocParser::make_NO_Q_LINE(yytext, yyextra->loc);
}
<QQ_STRING_STATE>[^\"\n\\]+ {
    return noise::DocParser::make_NO_QQ_LINE(yytext, yyextra->loc);
}
 /* A quoted value may span multiple lines: without this, flex's default
  * action (echo the unmatched char straight to yyout) would swallow each
  * embedded newline - dropping it from the token stream instead of the
  * captured value, and leaking it into the output ahead of everything
  * emitted through extra->emit(). */
<Q_STRING_STATE>\n {
    yyextra->loc.lines(1);
    return noise::DocParser::make_NO_Q_LINE(yytext, yyextra->loc);
}
<QQ_STRING_STATE>\n {
    yyextra->loc.lines(1);
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

 /* '\' always escapes exactly the next character, everywhere: plain document
  * text, inside a quoted span, inside a param/arg value, inside a literal
  * bracket group. There is no context where a bare '\' survives as itself -
  * a literal '\' in the output needs "\\", same as any other char that
  * would otherwise be read for its usual meaning (a macro name, an attr
  * keyword, '<'/'('/'#', a quote that would otherwise close the span, ...).
  * Returning the escaped character as its own one-character token (rather
  * than post-processing a captured run with unescape()) is what makes this
  * work uniformly: it breaks whatever longer match the escaped character
  * would otherwise have participated in, so e.g. "\repeat" can never lex as
  * the macro name "repeat" - only the un-escaped remainder "epeat" is left
  * to (fail to) match anything special. */
<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE,PARAM_VALUES_STATE,ARG_VALUES_STATE,LP_STATE,LA_STATE,Q_STRING_STATE,QQ_STRING_STATE>\\(.|\n) {
    if (yytext[1] == '\n') yyextra->loc.lines(1);
    IN_VALUE;
    char c = yytext[1] == '$' ? noise::LITERAL_DOLLAR : yytext[1];
    return noise::DocParser::make_ESCAPED_CHAR(std::string(1, c), yyextra->loc);
}

 /* -- rules shared by INITIAL and the two "peeking" states -- */

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\$(param|arg)\[[0-9]+\] {
    return expand_name(yyextra, yytext + 1);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>#(params|args|line|col|id) {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return TEXT_TOKEN(yytext);
}

 /* Same lookups, but a value's raw content is a NO_COMMA_TEXT, not a TEXT -
  * balanced_text has no alternative accepting a bare TEXT token. */
<PARAM_VALUES_STATE,ARG_VALUES_STATE>\$(param|arg)\[[0-9]+\] {
    IN_VALUE;
    return value_name(yyextra, yytext);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>#(params|args|line|col|id) {
    IN_VALUE;
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\${ID} {
    if (yyextra->is_macro(yytext + 1)) {
        return noise::DocParser::make_MACRO(yytext + 1, yyextra->loc);
    }
    return expand_name(yyextra, yytext + 1);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\n {
    yyextra->loc.lines(1);
    return TEXT_TOKEN(yytext);
}

<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>[\'\"] {
    /* The Q_/QQ_STRING_STATE push is done by the grammar (see quoted_text),
     * after the parser has finished with any pending peek-state reductions:
     * pushing it here would sit on top of the peek state and be popped in
     * its place. */
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
    return TEXT_TOKEN(yytext);
}

<INITIAL>[^\n'\"#\\$]+ {
    return TEXT_TOKEN(yytext);
}
<EXPECT_PARAMS_STATE>[^\n'\"#<(>),\\$]+ {
    return TEXT_TOKEN(yytext);
}
<EXPECT_ARGS_STATE>[^\n'\"#(>),\\$]+ {
    return TEXT_TOKEN(yytext);
}

 /* A '$' not starting a name ("$5", "US$ 3") is just literal text. */
<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE>\$ {
    return TEXT_TOKEN(yytext);
}

 /* A lone '#' not starting one of the four builtins is just literal text. */
<INITIAL,EXPECT_PARAMS_STATE,EXPECT_ARGS_STATE># {
    return TEXT_TOKEN(yytext);
}

 /* -- ParamList/ArgList content: only reached once '<' or '(' was actually seen -- */

 /* Whitespace (including newlines) is an ordinary character of a value. The
  * only exceptions are syntactic separators: whitespace before an attr
  * keyword or a "name=" at the head of an item, and whitespace between a
  * name and its '='. */
<PARAM_VALUES_STATE,ARG_VALUES_STATE>{
[ \t\n]+/required          { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/keep_left_ws      { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/klws              { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/keep_right_ws     { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/krws              { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/keep_enclosing_ws { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/kws               { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/quoted            { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/bool              { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/{ID}[ \t\n]*=     { WS_BEFORE_HEAD_TOKEN; }
[ \t\n]+/= {
    for (int i = 0; i < yyleng; ++i) if (yytext[i] == '\n') yyextra->loc.lines(1);
    if (yyextra->after_id) { yyextra->loc.step(); }
    else { IN_VALUE; return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc); }
}
[ \t\n]+ {
    for (int i = 0; i < yyleng; ++i) if (yytext[i] == '\n') yyextra->loc.lines(1);
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{
required|keep_left_ws|klws|keep_right_ws|krws|keep_enclosing_ws|kws|quoted|bool {
    if (!yyextra->item_head) {
        IN_VALUE;
        return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
    }
    string k(yytext);
    for (auto& c : k) c = std::tolower(c);
    if (k == "required") return noise::DocParser::make_REQUIRED(yyextra->loc);
    if (k == "keep_left_ws" || k == "klws")
        return noise::DocParser::make_KEEP_LEFT_WS(yyextra->loc);
    if (k == "keep_right_ws" || k == "krws")
        return noise::DocParser::make_KEEP_RIGHT_WS(yyextra->loc);
    if (k == "keep_enclosing_ws" || k == "kws")
        return noise::DocParser::make_KEEP_ENCLOSING_WS(yyextra->loc);
    if (k == "bool") return noise::DocParser::make_BOOL(yyextra->loc);
    return noise::DocParser::make_QUOTED(yyextra->loc);
}
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{ID}/[ \t\n]*= {
    if (yyextra->item_head) {
        yyextra->item_head = false;
        yyextra->after_id = true;
        return noise::DocParser::make_ID(yytext, yyextra->loc);
    }
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

 /* Only "$name" is a name. Inside a value it is substituted right away if it
  * is in scope; otherwise - a macro call, or a name bound only later (e.g. a
  * foreach loop variable) - it stays raw "$name" text: balanced_text just
  * captures the value, which is expanded later (params: right away; args:
  * once the consuming macro references them). A plain name at the head of an
  * item is an ID token (a flag implying "=true", see the `param`/`arg`
  * grammar) only when it names a BOOL-attributed formal of the macro being
  * called; anything else is plain text. */
<PARAM_VALUES_STATE,ARG_VALUES_STATE>\${ID} {
    IN_VALUE;
    return value_name(yyextra, yytext);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>{ID} {
    if (yyextra->item_head && yyextra->is_bool_formal(yytext)) {
        IN_VALUE;
        return noise::DocParser::make_ID(yytext, yyextra->loc);
    }
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>, {
    yyextra->item_head = true;
    yyextra->after_id = false;
    return noise::DocParser::symbol_type(',', yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>= {
    if (yyextra->after_id) {
        yyextra->after_id = false;
        return noise::DocParser::symbol_type('=', yyextra->loc);
    }
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>[\'\"] {
    IN_VALUE;
    return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
}

 /* A literal (non-macro-call) bracket group inside a value: push a state of
  * its own so its matching close pops back here instead of being mistaken
  * for the enclosing list's own '>' / ')'. See the comment by the state
  * declarations above. */
<PARAM_VALUES_STATE,ARG_VALUES_STATE,LP_STATE,LA_STATE>\( {
    IN_VALUE;
    doc_push_LP_STATE(yyscanner);
    return noise::DocParser::symbol_type('(', yyextra->loc);
}
<PARAM_VALUES_STATE,ARG_VALUES_STATE,LP_STATE,LA_STATE>\< {
    IN_VALUE;
    doc_push_LA_STATE(yyscanner);
    return noise::DocParser::symbol_type('<', yyextra->loc);
}

<LP_STATE>\) {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type(')', yyextra->loc);
}
<LA_STATE>\> {
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type('>', yyextra->loc);
}

<LP_STATE,LA_STATE>, {
    return noise::DocParser::symbol_type(',', yyextra->loc);
}

<LP_STATE,LA_STATE>\$(param|arg)\[[0-9]+\] {
    return value_name(yyextra, yytext);
}

<LP_STATE,LA_STATE>#(params|args|line|col|id) {
    std::optional<std::string> cv = yyextra->context_value(yytext);
    if (cv) {
        return noise::DocParser::make_CONTEXT_VALUE(*cv, yyextra->loc);
    }
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<LP_STATE,LA_STATE>\${ID} {
    return value_name(yyextra, yytext);
}

<LP_STATE,LA_STATE>[\'\"] {
    return noise::DocParser::symbol_type(yytext[0], yyextra->loc);
}

<LP_STATE,LA_STATE>\n {
    yyextra->loc.lines(1);
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<LP_STATE,LA_STATE>[^,'\"<>()#\\\n$]+ {
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<LP_STATE,LA_STATE>\$ {
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

 /* A lone '#' not starting one of the four builtins above is just literal
  * content (falls through here since the specific rule above always wins
  * on length when it does match). */
<LP_STATE,LA_STATE># {
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE>> {
    IN_VALUE;
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type('>', yyextra->loc);
}
<ARG_VALUES_STATE>\) {
    IN_VALUE;
    doc_pop_state(yyscanner);
    return noise::DocParser::symbol_type(')', yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>[^,'\"=<>()#\\\n \t$]+ {
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

<PARAM_VALUES_STATE,ARG_VALUES_STATE>\$ {
    IN_VALUE;
    return noise::DocParser::make_NO_COMMA_TEXT(yytext, yyextra->loc);
}

 /* A lone '#' not starting one of the four builtins above is just literal
  * content (falls through here since the specific rule above always wins
  * on length when it does match). */
<PARAM_VALUES_STATE,ARG_VALUES_STATE># {
    IN_VALUE;
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
