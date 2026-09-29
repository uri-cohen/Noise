/* Copyrights Uri Cohen uri.l.cohen@gmail.com 2026 */
/* clang-format off */

%code requires {

#include <NoiseFlow.h>
#include <Macro.h>
#include <Context.h>
#include <Binding.h>
#include <Logger.h>

#include <DocParser.tab.hh>
#include <DocLexerExtra.h>

typedef noise::DocLexerExtra DocLexerExtra;

}

%skeleton "lalr1.cc" /* Enables true C++ classes */
%require "3.8"
%language "c++"
%header
%locations

%define api.namespace {noise}
%define api.parser.class {DocParser}
%define api.location.file "doc_location.hh"

%define api.value.type variant
%define api.token.constructor

%lex-param { yyscan_t yyscanner }
%parse-param { yyscan_t yyscanner }
%parse-param { noise::DocLexerExtra* extra }

%code {
#include <Exception.h>
#include <Utils.h>
#include <VarSolver.h>

#define yylex doc_lex
noise::DocParser::symbol_type yylex(yyscan_t yyscanner);

void noise::DocParser::error(const location_type& l, const std::string& msg) {
    ERROR(201, std::format("DocParser: {}:{}.{}-{}.{}: {}",
                           *(l.begin.filename), l.begin.line, l.begin.column,
                           l.end.line, l.end.column, msg));
};

static std::string lowered(const std::string& s) {
    std::string out = s;
    for (auto& c : out) c = std::tolower(static_cast<unsigned char>(c));
    return out;
}

// Case-insensitive true/false spellings for a BOOL-attributed param/arg.
// Anything else warns and falls back to the formal's own default (itself
// normalized the same way; "false" if that isn't a valid spelling either).
static std::string normalize_bool(const std::string& raw, const std::string& formal_default) {
    static const std::set<std::string> TRUE_STRINGS = {"true", "t", "yes", "y", "1", "ok"};
    static const std::set<std::string> FALSE_STRINGS = {"false", "f", "no", "n", "0"};
    std::string v = lowered(raw);
    if (TRUE_STRINGS.contains(v)) return "true";
    if (FALSE_STRINGS.contains(v)) return "false";
    WARNING(301, "invalid bool value '{}', using default '{}'", raw, formal_default);
    return TRUE_STRINGS.contains(lowered(formal_default)) ? "true" : "false";
}

static std::string apply_attrs(const std::string& value, noise::BindingAttr attrs,
                                const std::string& bool_default = "false") {
    std::string v = value;
    if (noise::has(attrs, noise::BindingAttr::QUOTED) && v.size() >= 2) {
        char front = v.front(), back = v.back();
        if ((front == '"' && back == '"') || (front == '\'' && back == '\'')) {
            v = v.substr(1, v.size() - 2);
        }
    }
    // Trimming is the default; KEEP_LEFT_WS/KEEP_RIGHT_WS opt out per side.
    bool keep_left = noise::has(attrs, noise::BindingAttr::KEEP_LEFT_WS);
    bool keep_right = noise::has(attrs, noise::BindingAttr::KEEP_RIGHT_WS);
    v = noise::trim(v, keep_left, keep_right);
    if (noise::has(attrs, noise::BindingAttr::BOOL)) {
        v = normalize_bool(v, bool_default);
    }
    return v;
}

static std::string apply_attrs(const noise::Binding& b) {
    return apply_attrs(b.value(), b.attrs());
}

// Expansion Flow step 5: instantiation context mapping shared by every
// macro type (see NoiseFlow::bind_instance).
static void bind_list(const std::string& macro_name,
                       const std::vector<noise::Binding>& formals,
                       const std::vector<noise::Binding>& actuals,
                       const std::string& prefix, noise::Context& ctx) {
    std::vector<bool> resolved(formals.size(), false);
    std::vector<std::string> values(formals.size());
    std::set<std::string> claimed_names;

    // A call-site attr (e.g. keep_left_ws on a specific actual) applies
    // alongside whatever the formal itself declares - either side asking to
    // keep/quote is enough, so the two attr sets are combined, not one
    // overriding the other.
    for (const auto& a : actuals) {
        if (a.name().empty()) continue;
        for (size_t i = 0; i < formals.size(); ++i) {
            if (!resolved[i] && formals[i].name() == a.name()) {
                values[i] = apply_attrs(a.value(), formals[i].attrs() | a.attrs(),
                                        formals[i].value());
                resolved[i] = true;
                claimed_names.insert(a.name());
                break;
            }
        }
    }
    size_t pos_idx = 0;
    for (size_t i = 0; i < formals.size(); ++i) {
        if (resolved[i]) continue;
        while (pos_idx < actuals.size() && !actuals[pos_idx].name().empty()) {
            ++pos_idx;
        }
        if (pos_idx < actuals.size()) {
            const noise::Binding& a = actuals[pos_idx];
            values[i] = apply_attrs(a.value(), formals[i].attrs() | a.attrs(),
                                    formals[i].value());
            resolved[i] = true;
            ++pos_idx;
        }
    }
    for (size_t i = 0; i < formals.size(); ++i) {
        if (!resolved[i]) {
            if (formals[i].required()) {
                throw noise::NoiseMacroCallError(std::format(
                    "missing required {} '{}' for macro '{}'", prefix,
                    formals[i].name(), macro_name));
            }
            values[i] = formals[i].value();
            if (formals[i].is_bool()) {
                values[i] = normalize_bool(values[i], formals[i].value());
            }
        }
        ctx.add_map(formals[i].name(), values[i]);
        ctx.add_map(std::format("{}[{}]", prefix, i), values[i]);
    }
    for (size_t i = 0; i < actuals.size(); ++i) {
        std::string v = apply_attrs(actuals[i]);
        ctx.add_map(std::format("{}[{}]", prefix, i), v);
        if (!actuals[i].name().empty() && !claimed_names.contains(actuals[i].name())) {
            ctx.add_map(actuals[i].name(), v);
        }
    }
    ctx.add_map(std::format("#{}s", prefix), std::to_string(actuals.size()));
}

// Expansion Flow steps 7-8: re-expands `raw` as a fresh document, repeating
// until the output stops changing, or throws if it doesn't within
// max_expansions (NOISE_MAX_EXPANSIONS) rounds. Used both for a macro's own
// body (post step 6) and, for params, right after balanced_text captures
// their raw value (step 4).
static std::string expand_until_stable(noise::DocLexerExtra* extra,
                                        const std::string& raw,
                                        const std::string& stream_id,
                                        int64_t max_expansions) {
    std::string prev = raw, curr;
    int64_t iterations = 0;
    do {
        std::ostringstream tmp;
        extra->owner->stream_expand(prev, stream_id, tmp);
        curr = tmp.str();
        if (curr == prev) {
            break;
        }
        prev = curr;
    } while (++iterations < max_expansions);
    if (iterations >= max_expansions) {
        throw noise::NoiseMacroCallError(std::format(
            "possible infinite expansion while expanding '{}' - not stable "
            "after NOISE_MAX_EXPANSIONS={} re-expansions", stream_id, max_expansions));
    }
    return prev;
}

// Counts one macro expansion nesting level for the guard's lifetime.
struct DepthGuard {
    explicit DepthGuard(int64_t& depth) : _depth(depth) { ++_depth; }
    ~DepthGuard() { --_depth; }
    int64_t& _depth;
};

static std::string invoke_macro(noise::DocLexerExtra* extra, const std::string& name,
                                 std::vector<noise::Binding> params,
                                 std::vector<noise::Binding> args) {
    noise::Macro* macro = extra->owner->find_macro(name);
    if (!macro) {
        throw noise::NoiseMacroCallError(std::format("unknown macro '{}'", name));
    }
    noise::Context ctx;
    bind_list(name, macro->params(), params, "param", ctx);
    bind_list(name, macro->args(), args, "arg", ctx);
    ctx.add_map("#line", std::to_string(extra->out_line));
    ctx.add_map("#col", std::to_string(extra->out_col));

    noise::ContextManager* cm = extra->owner->context_manager();
    cm->push(std::move(ctx));
    // Read once the call's own params are bound, so a call site param
    // (m<NOISE_MAX_DEPTH=...>) already applies to this very call.
    int64_t max_depth = cm->config_int(noise::config::MAX_DEPTH,
                                       noise::config::MAX_DEPTH_DEFAULT, 1);
    int64_t max_expansions = cm->config_int(noise::config::MAX_EXPANSIONS,
                                            noise::config::MAX_EXPANSIONS_DEFAULT, 1);
    if (extra->owner->depth() >= max_depth) {
        throw noise::NoiseMacroCallError(std::format(
            "macro '{}' exceeds the expansion nesting depth limit "
            "NOISE_MAX_DEPTH={} (a macro expanding itself?)", name, max_depth));
    }
    DepthGuard depth_guard(extra->owner->depth());
    // VARS are resolved right after the params (their constraints may refer
    // to them) and are then visible to the body just like params.
    bool has_vars = macro->vars() != nullptr;
    if (has_vars) {
        noise::Context vars_ctx;
        for (const auto& [k, v] : extra->owner->var_solver()->resolve(*macro->vars(), cm, extra->owner->gen())) {
            vars_ctx.add_map(k, v);
        }
        cm->push(std::move(vars_ctx));
    }
    std::string raw = macro->expand(cm);
    if (has_vars) {
        cm->pop();
    }
    cm->pop();

    return expand_until_stable(extra, raw, std::format("macro:{}", name), max_expansions);
}

}

%token END_OF_FILE 0
%token <std::string> TEXT
%token <std::string> MACRO
%token <std::string> CONTEXT_VALUE
%token <std::string> NO_COMMA_TEXT
%token <std::string> NO_Q_LINE
%token <std::string> NO_QQ_LINE
%token <std::string> ESCAPED_CHAR
%token <std::string> ID
%token REQUIRED
%token KEEP_LEFT_WS
%token KEEP_RIGHT_WS
%token KEEP_ENCLOSING_WS
%token QUOTED
%token BOOL

%nterm document
%nterm <std::string> expandable
%nterm <std::vector<noise::Binding>> opt_params
%nterm <std::vector<noise::Binding>> opt_args
%nterm <std::vector<noise::Binding>> param_list
%nterm <std::vector<noise::Binding>> arg_list
%nterm <noise::Binding> param
%nterm <noise::Binding> arg
%nterm <noise::BindingAttr> opt_attrs
%nterm <std::string> opt_id_eq
%nterm <std::string> balanced_text
%nterm <std::string> balanced_text_list
%nterm <std::string> quoted_text
%nterm <std::string> quoted_text_verbatim
%nterm <std::string> no_q_lines
%nterm <std::string> no_qq_lines

%start document

%%

document:
    %empty
    | document TEXT { extra->emit($2); }
    | document ESCAPED_CHAR { extra->emit($2); }
    | document expandable { extra->emit($2); }
    | document quoted_text_verbatim { extra->emit($2); }
    ;

/* A quoted span never has its content interpreted as a macro reference; at
 * document level (unlike inside a param/arg value) the quote characters
 * themselves are part of the literal text and are echoed verbatim. Its
 * content needs no unescape() pass: NO_Q_LINE/NO_QQ_LINE never contain a
 * '\' (see DocLexer.l), and any escape within the span already arrived
 * resolved, as its own ESCAPED_CHAR token. */
quoted_text_verbatim:
    '\'' { doc_push_Q_STRING_STATE(yyscanner); } no_q_lines '\'' {
        $$ = "'" + $3 + "'";
    }
    | '"' { doc_push_QQ_STRING_STATE(yyscanner); } no_qq_lines '"' {
        $$ = "\"" + $3 + "\"";
    }
    ;

expandable:
    MACRO {
        // Lets the lexer recognize a bare "name" entry (no "=value") as a
        // flag for one of *this* macro's own BOOL-attributed formals (see
        // is_bool_formal / the {ID} rule in DocLexer.l).
        extra->pending_macro_name = $1;
        doc_push_EXPECT_PARAMS_STATE(yyscanner);
    } opt_params {
        doc_pop_state(yyscanner);
        doc_push_EXPECT_ARGS_STATE(yyscanner);
    } opt_args {
        doc_pop_state(yyscanner);
        $$ = invoke_macro(extra, $1, $3, $5);
    }
    | CONTEXT_VALUE {
        $$ = $1;
    }
    ;

opt_params:
    %empty { $$ = std::vector<noise::Binding>(); }
    | '<' {
        doc_push_PARAM_VALUES_STATE(yyscanner);
        extra->item_head = true;
        extra->after_id = false;
    } param_list '>' {
        $$ = $3;
    }
    ;

opt_args:
    %empty { $$ = std::vector<noise::Binding>(); }
    | '(' {
        doc_push_ARG_VALUES_STATE(yyscanner);
        extra->item_head = true;
        extra->after_id = false;
    } arg_list ')' {
        $$ = $3;
    }
    ;

param_list:
    param { $$ = std::vector<noise::Binding>{$1}; }
    | param_list ',' param { $$ = $1; $$.push_back($3); }
    ;

arg_list:
    arg { $$ = std::vector<noise::Binding>{$1}; }
    | arg_list ',' arg { $$ = $1; $$.push_back($3); }
    ;

/* Expansion Flow step 4: a param's raw captured text is expanded right away,
 * so its formal binding sees the fully-resolved value (see bind_list). An
 * arg's raw text is kept as-is (step 2's "passed as is") - it is only ever
 * expanded later, if and when the invoked macro's body references it. */
/* A bare name - no "=value" at all - is only ever lexed as a standalone ID
 * here when it names a BOOL-attributed formal of the macro being called
 * (see is_bool_formal); anything else still lexes as ordinary balanced_text
 * (context lookup, then plain positional text), so this can't collide with
 * passing a bare word as a positional value. */
param:
    opt_attrs opt_id_eq balanced_text {
        int64_t max_expansions = extra->owner->context_manager()->config_int(
            noise::config::MAX_EXPANSIONS, noise::config::MAX_EXPANSIONS_DEFAULT, 1);
        $$ = noise::Binding($2, expand_until_stable(extra, $3, "param", max_expansions), $1);
    }
    | opt_attrs ID { $$ = noise::Binding($2, "true", $1); }
    ;

arg:
    opt_attrs opt_id_eq balanced_text { $$ = noise::Binding($2, $3, $1); }
    | opt_attrs ID { $$ = noise::Binding($2, "true", $1); }
    ;

opt_attrs:
    %empty { $$ = noise::BindingAttr::NONE; }
    | opt_attrs REQUIRED { $$ = $1 | noise::BindingAttr::REQUIRED; }
    | opt_attrs KEEP_LEFT_WS { $$ = $1 | noise::BindingAttr::KEEP_LEFT_WS; }
    | opt_attrs KEEP_RIGHT_WS { $$ = $1 | noise::BindingAttr::KEEP_RIGHT_WS; }
    | opt_attrs KEEP_ENCLOSING_WS {
          $$ = $1 | noise::BindingAttr::KEEP_LEFT_WS |
               noise::BindingAttr::KEEP_RIGHT_WS;
      }
    | opt_attrs QUOTED { $$ = $1 | noise::BindingAttr::QUOTED; }
    | opt_attrs BOOL { $$ = $1 | noise::BindingAttr::BOOL; }
    ;

opt_id_eq:
    %empty { $$ = std::string(); }
    | ID '=' { $$ = $1; }
    ;

/* Captures a param/arg's raw source text verbatim - it does not itself
 * recognize or expand macro calls (see expandable's callers instead). A
 * "(...)" or "<...>" group found here need not be a macro call at all (e.g.
 * literal text such as "f(a,b)"): it is just balanced text, matched purely
 * to find its end without miscounting the enclosing list's own ',' / close,
 * and reproduced verbatim. balanced_text_list's own balanced_text can be
 * empty, so a bare "()" / "<>" is already covered without a separate rule. */
balanced_text:
    %empty { $$ = std::string(); }
    | balanced_text NO_COMMA_TEXT { $$ = $1 + $2; }
    | balanced_text ESCAPED_CHAR { $$ = $1 + $2; }
    | balanced_text '(' balanced_text_list ')' { $$ = $1 + "(" + $3 + ")"; }
    | balanced_text '<' balanced_text_list '>' { $$ = $1 + "<" + $3 + ">"; }
    | balanced_text quoted_text { $$ = $1 + $2; }
    // #line/#params-style lookups still resolve immediately (see the {ID}
    // rule comment in DocLexer.l) - their resolved text is spliced in here
    // like any other already-known piece of the captured value.
    | balanced_text CONTEXT_VALUE { $$ = $1 + $2; }
    ;

balanced_text_list:
    balanced_text { $$ = $1; }
    | balanced_text_list ',' balanced_text { $$ = $1 + "," + $3; }
    ;

quoted_text:
    '\'' { doc_push_Q_STRING_STATE(yyscanner); } no_q_lines '\'' {
        $$ = $3;
    }
    | '"' { doc_push_QQ_STRING_STATE(yyscanner); } no_qq_lines '"' {
        $$ = $3;
    }
    ;

no_q_lines:
    %empty { $$ = std::string(); }
    | no_q_lines NO_Q_LINE { $$ = $1 + $2; }
    | no_q_lines ESCAPED_CHAR { $$ = $1 + $2; }
    ;

no_qq_lines:
    %empty { $$ = std::string(); }
    | no_qq_lines NO_QQ_LINE { $$ = $1 + $2; }
    | no_qq_lines ESCAPED_CHAR { $$ = $1 + $2; }
    ;

%%
