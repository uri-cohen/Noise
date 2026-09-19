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

#define yylex doc_lex
noise::DocParser::symbol_type yylex(yyscan_t yyscanner);

void noise::DocParser::error(const location_type& l, const std::string& msg) {
    ERROR(201, std::format("DocParser: {}:{}.{}-{}.{}: {}",
                           *(l.begin.filename), l.begin.line, l.begin.column,
                           l.end.line, l.end.column, msg));
};

static std::string apply_attrs(const std::string& value, noise::BindingAttr attrs) {
    std::string v = value;
    if (noise::has(attrs, noise::BindingAttr::QUOTED) && v.size() >= 2) {
        char front = v.front(), back = v.back();
        if ((front == '"' && back == '"') || (front == '\'' && back == '\'')) {
            v = v.substr(1, v.size() - 2);
        }
    }
    if (noise::has(attrs, noise::BindingAttr::TRIM)) {
        v = noise::trim(v);
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

    for (const auto& a : actuals) {
        if (a.name().empty()) continue;
        for (size_t i = 0; i < formals.size(); ++i) {
            if (!resolved[i] && formals[i].name() == a.name()) {
                values[i] = apply_attrs(a.value(), formals[i].attrs());
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
            values[i] = apply_attrs(actuals[pos_idx].value(), formals[i].attrs());
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

    extra->owner->context_manager()->push(std::move(ctx));
    std::string raw = macro->expand(extra->owner->context_manager());
    extra->owner->context_manager()->pop();

    std::string prev = raw, curr;
    int iterations = 0;
    constexpr int MAX_ITERATIONS = 64;
    do {
        std::ostringstream tmp;
        extra->owner->stream_expand(prev, std::format("macro:{}", name), tmp);
        curr = tmp.str();
        if (curr == prev) {
            break;
        }
        prev = curr;
    } while (++iterations < MAX_ITERATIONS);
    if (iterations >= MAX_ITERATIONS) {
        throw noise::NoiseMacroCallError(std::format(
            "possible infinite expansion in macro '{}'", name));
    }
    return prev;
}

}

%token END_OF_FILE 0
%token <std::string> TEXT
%token <std::string> MACRO
%token <std::string> CONTEXT_VALUE
%token <std::string> NO_COMMA_TEXT
%token <std::string> NO_Q_LINE
%token <std::string> NO_QQ_LINE
%token <std::string> ID
%token REQUIRED
%token TRIM
%token QUOTED

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
%nterm <std::string> quoted_text
%nterm <std::string> quoted_text_verbatim
%nterm <std::string> no_q_lines
%nterm <std::string> no_qq_lines

%start document

%%

document:
    %empty
    | document TEXT { extra->emit($2); }
    | document expandable { extra->emit($2); }
    | document quoted_text_verbatim { extra->emit($2); }
    ;

/* A quoted span never has its content interpreted as a macro reference; at
 * document level (unlike inside a param/arg value) the quote characters
 * themselves are part of the literal text and are echoed verbatim. */
quoted_text_verbatim:
    '\'' no_q_lines '\'' {
        $$ = "'" + noise::unescape($2) + "'";
    }
    | '"' no_qq_lines '"' {
        $$ = "\"" + noise::unescape($2) + "\"";
    }
    ;

expandable:
    MACRO {
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
    } param_list '>' { $$ = $3; }
    ;

opt_args:
    %empty { $$ = std::vector<noise::Binding>(); }
    | '(' {
        doc_push_ARG_VALUES_STATE(yyscanner);
    } arg_list ')' { $$ = $3; }
    ;

param_list:
    param { $$ = std::vector<noise::Binding>{$1}; }
    | param_list ',' param { $$ = $1; $$.push_back($3); }
    ;

arg_list:
    arg { $$ = std::vector<noise::Binding>{$1}; }
    | arg_list ',' arg { $$ = $1; $$.push_back($3); }
    ;

param:
    opt_attrs opt_id_eq balanced_text { $$ = noise::Binding($2, $3, $1); }
    ;

arg:
    opt_attrs opt_id_eq balanced_text { $$ = noise::Binding($2, $3, $1); }
    ;

opt_attrs:
    %empty { $$ = noise::BindingAttr::NONE; }
    | opt_attrs REQUIRED { $$ = $1 | noise::BindingAttr::REQUIRED; }
    | opt_attrs TRIM { $$ = $1 | noise::BindingAttr::TRIM; }
    | opt_attrs QUOTED { $$ = $1 | noise::BindingAttr::QUOTED; }
    ;

opt_id_eq:
    %empty { $$ = std::string(); }
    | ID '=' { $$ = $1; }
    ;

balanced_text:
    %empty { $$ = std::string(); }
    | balanced_text NO_COMMA_TEXT { $$ = $1 + noise::unescape($2); }
    | balanced_text expandable { $$ = $1 + $2; }
    | balanced_text quoted_text { $$ = $1 + $2; }
    ;

quoted_text:
    '\'' no_q_lines '\'' {
        $$ = noise::unescape($2);
    }
    | '"' no_qq_lines '"' {
        $$ = noise::unescape($2);
    }
    ;

no_q_lines:
    %empty { $$ = std::string(); }
    | no_q_lines NO_Q_LINE { $$ = $1 + $2; }
    ;

no_qq_lines:
    %empty { $$ = std::string(); }
    | no_qq_lines NO_QQ_LINE { $$ = $1 + $2; }
    ;

%%
