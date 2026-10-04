/* Copyrights Uri Cohen uri.l.cohen@gmail.com 2026 */
/* clang-format off */

%code requires {

#include <NoiseFlow.h>
#include <TextMacro.h>
#include <Binding.h>
#include <Logger.h>
#include <Vars.h>

#include <DefParser.tab.hh>
#include <DefLexerExtra.h>
//

typedef noise::DefLexerExtra DefLexerExtra;

}

%skeleton "lalr1.cc" /* Enables true C++ classes */
%require "3.8"
%language "c++"
%header
%locations

%define api.namespace {noise}
%define api.parser.class {DefParser}
%define api.location.file "def_location.hh"

/* Define the signature of yylex so it accepts Bison's modern C++ types */
%define api.value.type variant
%define api.token.constructor

%lex-param { yyscan_t yyscanner }
%parse-param { yyscan_t yyscanner }
%parse-param { noise::DefLexerExtra* extra }

/* Declare your custom lexer signature to match */
%code {
#include <Exception.h>
#include <Utils.h>

#define yylex def_lex
noise::DefParser::symbol_type yylex(yyscan_t yyscanner);

void noise::DefParser::error(const location_type& l, const std::string& msg) {
    ERROR(201, std::format("DefBuilder: {}:{}.{}-{}.{}: {}",
                           *(l.begin.filename), l.begin.line, l.begin.column,
                           l.end.line, l.end.column, msg));
}; /* error */

void begin_macro(noise::DefLexerExtra* extra, const std::string& name) {
    extra->curr_name = name;
    extra->curr_text.clear();
    extra->curr_text_set = false;
    extra->curr_params.clear();
    extra->curr_args.clear();
    extra->curr_macro_vars.reset();
    extra->curr_exports.clear();
}

void end_macro(noise::DefLexerExtra* extra) {
    if (!extra->curr_text_set) {
        throw noise::NoiseDefBuilderError(
            std::format("macro '{}' missing TEXT", extra->curr_name));
    }
    // owned here until stored: add_param/add_arg/add_export may throw
    auto m = std::make_unique<noise::TextMacro>(extra->curr_name, extra->curr_text);
    for (auto& p : extra->curr_params) {
        m->add_param(p);
    }
    for (auto& a : extra->curr_args) {
        m->add_arg(a);
    }
    m->set_vars(extra->curr_macro_vars);
    for (auto& e : extra->curr_exports) {
        m->add_export(e);
    }
    extra->macro_list.push_back(m.release());
}

static noise::ExprPtr binary(noise::Expr::Op op, noise::ExprPtr a, noise::ExprPtr b) {
    return noise::Expr::binary(op, a, b);
}

void finalize(noise::DefLexerExtra* extra) {
    for (auto m : extra->macro_list) {
        m->set_owner(extra->owner);
        extra->owner->add_macro(m);
    }
    extra->macro_list.clear();
    extra->delimiter = "";
}


} /* %code */

/* ************************************************************************** */

%token END_OF_FILE 0
%token <std::string> ID
%token LEFT_SHIFT
%token <std::string> ANY_STRING_LINE
%token <std::string> DELIMITER
%token <std::string> FILE_NAME
%token <std::string> BARE_STRING
%token MACRO
%token INCLUDE
%token TEXT
%token PARAMS
%token ARGS
%token REQUIRED
%token KEEP_LEFT_WS
%token KEEP_RIGHT_WS
%token KEEP_ENCLOSING_WS
%token QUOTED
%token BOOL
%token COLON_EQ
%token VARS
%token EXPORT
%token GLOBAL
%token CALLER
%token INT_T
%token UINT_T
%token REAL_T
%token BITVEC_T
%token STRING_T
%token ASSERT
%token SMTLIB
%token AND
%token OR
%token XOR
%token NOT
%token SHR
%token LE
%token GE
%token EQ
%token NE
%token IMPLIES
%token <std::string> LITERAL

/* constraint expression operators, lowest precedence first */
%right IMPLIES
%left OR
%left XOR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '<' '>' LE GE
%left LEFT_SHIFT SHR
%left '+' '-'
%left '*' '/' '%'
%precedence UNARY
%precedence '['      /* postfix bit select binds tightest: -b[0] is -(b[0]) */

%nterm def_file
%nterm defs
%nterm include
%nterm macro
%nterm macro_body
%nterm <std::string> no_double_q_string
%nterm <std::string> no_single_q_string
%nterm <std::string> quoted_string
%nterm <std::string> value
%nterm <std::string> here_string
%nterm <std::string> any_string
%nterm param_list
%nterm arg_list
%nterm <noise::BindingAttr> attrs
%nterm <noise::AssignMode> assign_op
%nterm var_items
%nterm var_decl_list
%nterm var_decl
%nterm <noise::VarType> var_type
%nterm <noise::ExprPtr> expr
%nterm <noise::ExprPtr> bit_index

%start def_file

%%

def_file: defs END_OF_FILE { finalize(extra); } ;

defs:
    %empty
    | defs include
    | defs macro
    | defs global_vars
    | defs error
    ;

include:
    INCLUDE '<' {
          def_push_INCLUDE_STATE(yyscanner);
      } FILE_NAME '>' {
          def_pop_state(yyscanner);
      } ';' {
          extra->owner->import_include($4, extra->including_dir);
      }
    ;

macro:
    MACRO ID '=' {
          begin_macro(extra, $2);
      } '{' macro_body '}' ';' {
          end_macro(extra);
      }
    ;

/* A top level VARS clause: global document scope variables. */
global_vars:
    VARS '=' {
          def_push_VARS_STATE(yyscanner);
          extra->curr_vars = std::make_shared<noise::VarBlock>();
      } '{' var_items '}' ';' {
          def_pop_state(yyscanner);
          extra->owner->add_global_vars(extra->curr_vars);
          extra->curr_vars.reset();
      }
    ;

macro_body:
    %empty
    | macro_body TEXT '=' quoted_string ';' {
          extra->curr_text = $4;
          extra->curr_text_set = true;
      }
    | macro_body PARAMS '=' {
          def_push_PARAM_LIST_STATE(yyscanner);
      } '{' param_list '}' ';' {
          def_pop_state(yyscanner);
      }
    | macro_body ARGS '=' {
          def_push_ARG_LIST_STATE(yyscanner);
      } '{' arg_list '}' ';' {
          def_pop_state(yyscanner);
      }
    | macro_body EXPORT '=' {
          def_push_EXPORT_STATE(yyscanner);
      } '{' export_list '}' ';' {
          def_pop_state(yyscanner);
      }
    | macro_body VARS '=' {
          def_push_VARS_STATE(yyscanner);
          if (!extra->curr_macro_vars) {
              extra->curr_macro_vars = std::make_shared<noise::VarBlock>();
          }
          extra->curr_vars = extra->curr_macro_vars;
      } '{' var_items '}' ';' {
          def_pop_state(yyscanner);
          extra->curr_vars.reset();
      }
    ;

export_list:
    %empty
    | export_list ID {
          extra->curr_export = noise::Export{};
          extra->curr_export.name = $2;
      } export_scope export_value ';' {
          extra->curr_exports.push_back(extra->curr_export);
      }
    ;

/* none: the caller's scope */
export_scope:
    %empty
    | CALLER { extra->curr_export.scope = noise::Export::Scope::CALLER; }
    | GLOBAL { extra->curr_export.scope = noise::Export::Scope::GLOBAL; }
    | '{' { extra->curr_export.scope = noise::Export::Scope::MACROS; } export_macros '}'
    ;

export_macros:
    ID { extra->curr_export.macros.push_back($1); }
    | export_macros ',' ID { extra->curr_export.macros.push_back($3); }
    ;

/* none: the name's current value (in the exporting call), else "true" */
export_value:
    %empty
    | '=' {
          def_push_DEF_VALUE_STATE(yyscanner);
      } value {
          def_pop_state(yyscanner);
          extra->curr_export.value = $3;
      }
    ;

var_items:
    %empty
    | var_items var_type {
          extra->curr_var_type = $2;
      } var_decl_list ';'
    | var_items ASSERT expr ';' {
          extra->curr_vars->add_assert($3);
      }
    | var_items SMTLIB quoted_string ';' {
          extra->curr_vars->add_smtlib($3);
      }
    ;

var_decl_list:
    var_decl
    | var_decl_list ',' var_decl
    ;

var_decl:
    ID {
          extra->curr_vars->add_decl($1, extra->curr_var_type, std::nullopt);
      }
    | ID '=' {
          def_push_VAR_VALUE_STATE(yyscanner);
      } value {
          def_pop_state(yyscanner);
          extra->curr_vars->add_decl($1, extra->curr_var_type, $4);
      }
    ;

var_type:
    BOOL       { $$ = noise::VarType{noise::VarKind::BOOL}; }
    | INT_T    { $$ = noise::VarType{noise::VarKind::INT}; }
    | UINT_T   { $$ = noise::VarType{noise::VarKind::UINT}; }
    | REAL_T   { $$ = noise::VarType{noise::VarKind::REAL}; }
    | STRING_T { $$ = noise::VarType{noise::VarKind::STRING}; }
    | BITVEC_T '[' LITERAL ']' {
          auto w = noise::parse_uint64($3);
          if (!w || *w == 0 || *w > 65536) {
              throw noise::NoiseDefBuilderError(
                  std::format("invalid BITVEC width '{}'", $3));
          }
          $$ = noise::VarType{noise::VarKind::BITVEC, static_cast<unsigned>(*w)};
      }
    ;

/* An identifier declared earlier in the same VARS clause is a variable;
 * any other is a name, resolved at solve time (see VarSolver.h). */
expr:
    LITERAL { $$ = noise::Expr::leaf(noise::Expr::Op::LITERAL, $1); }
    | quoted_string { $$ = noise::Expr::leaf(noise::Expr::Op::LITERAL, $1); }
    | ID {
          $$ = noise::Expr::leaf(extra->curr_vars->declared($1)
                                     ? noise::Expr::Op::VAR
                                     : noise::Expr::Op::NAME, $1);
      }
    | '(' expr ')' { $$ = $2; }
    | '(' var_type ')' expr %prec UNARY { $$ = noise::Expr::cast($2, $4); }
    /* Verilog-like bit select of a BITVEC: b[i] and b[hi:lo] (hi >= lo, both
     * included); typing and range checks happen at solve time. */
    | expr '[' bit_index ']' { $$ = binary(noise::Expr::Op::INDEX, $1, $3); }
    | expr '[' bit_index ':' bit_index ']' { $$ = noise::Expr::slice($1, $3, $5); }
    | '-' expr %prec UNARY { $$ = noise::Expr::unary(noise::Expr::Op::NEG, $2); }
    | '~' expr %prec UNARY { $$ = noise::Expr::unary(noise::Expr::Op::BITNOT, $2); }
    | NOT expr %prec UNARY { $$ = noise::Expr::unary(noise::Expr::Op::NOT, $2); }
    | expr '*' expr        { $$ = binary(noise::Expr::Op::MUL, $1, $3); }
    | expr '/' expr        { $$ = binary(noise::Expr::Op::DIV, $1, $3); }
    | expr '%' expr        { $$ = binary(noise::Expr::Op::MOD, $1, $3); }
    | expr '+' expr        { $$ = binary(noise::Expr::Op::ADD, $1, $3); }
    | expr '-' expr        { $$ = binary(noise::Expr::Op::SUB, $1, $3); }
    | expr LEFT_SHIFT expr { $$ = binary(noise::Expr::Op::SHL, $1, $3); }
    | expr SHR expr        { $$ = binary(noise::Expr::Op::SHR, $1, $3); }
    | expr '<' expr        { $$ = binary(noise::Expr::Op::LT, $1, $3); }
    | expr LE expr         { $$ = binary(noise::Expr::Op::LE, $1, $3); }
    | expr '>' expr        { $$ = binary(noise::Expr::Op::GT, $1, $3); }
    | expr GE expr         { $$ = binary(noise::Expr::Op::GE, $1, $3); }
    | expr EQ expr         { $$ = binary(noise::Expr::Op::EQ, $1, $3); }
    | expr NE expr         { $$ = binary(noise::Expr::Op::NE, $1, $3); }
    | expr '&' expr        { $$ = binary(noise::Expr::Op::BITAND, $1, $3); }
    | expr '^' expr        { $$ = binary(noise::Expr::Op::BITXOR, $1, $3); }
    | expr '|' expr        { $$ = binary(noise::Expr::Op::BITOR, $1, $3); }
    | expr AND expr        { $$ = binary(noise::Expr::Op::AND, $1, $3); }
    | expr XOR expr        { $$ = binary(noise::Expr::Op::XOR, $1, $3); }
    | expr OR expr         { $$ = binary(noise::Expr::Op::OR, $1, $3); }
    | expr IMPLIES expr    { $$ = binary(noise::Expr::Op::IMPLIES, $1, $3); }
    ;

/* A bit select index: an integer literal, or a name resolved at solve time
 * (a param); a variable is rejected then, as Z3's extract needs constants. */
bit_index:
    LITERAL { $$ = noise::Expr::leaf(noise::Expr::Op::LITERAL, $1); }
    | ID {
          $$ = noise::Expr::leaf(extra->curr_vars->declared($1)
                                     ? noise::Expr::Op::VAR
                                     : noise::Expr::Op::NAME, $1);
      }
    ;

quoted_string:
    '"' {
        def_push_QQ_STRING_STATE(yyscanner);
    } no_double_q_string '"' {
        $$ = noise::unescape($3);
        def_pop_state(yyscanner);
    }
    | '\'' {
        def_push_Q_STRING_STATE(yyscanner);
    } no_single_q_string '\'' {
        $$ = noise::unescape($3);
        def_pop_state(yyscanner);
    }
    | here_string {
        $$ = noise::unescape($1);
    }
    ;

no_double_q_string:
    %empty { $$ = std::string(); }
    | no_double_q_string ANY_STRING_LINE {
        $$ = $1 + $2;
    }

no_single_q_string:
    %empty {
        $$ = std::string();
    }
    | no_single_q_string ANY_STRING_LINE {
        $$ = $1 + $2;
    }

here_string:
    LEFT_SHIFT ID {
         extra->delimiter = $2;
         def_push_HERE_STRING_STATE(yyscanner);
    } any_string DELIMITER {
         $$ = $4;
         def_pop_state(yyscanner);
    }
    ;

any_string:
    %empty { $$ = std::string(); }
    | any_string ANY_STRING_LINE {
          $$ = $1 + $2;
      }

param_list:
    %empty
    | param_list attrs ID assign_op {
          def_push_DEF_VALUE_STATE(yyscanner);
      } value ';' {
          def_pop_state(yyscanner);
          extra->curr_params.emplace_back($3, $6, $2, $4);
      }
    ;

arg_list:
    %empty
    | arg_list attrs ID assign_op {
          def_push_DEF_VALUE_STATE(yyscanner);
      } value ';' {
          def_pop_state(yyscanner);
          extra->curr_args.emplace_back($3, $6, $2, $4);
      }
    ;

/* "name := value" in a definition: every caller's normal assignment of it is
 * conditional - done only if the name isn't defined already (see bind_list in
 * DocParser.yy); a caller's "=!" still forces it. */
assign_op:
    '='        { $$ = noise::AssignMode::NORMAL; }
    | COLON_EQ { $$ = noise::AssignMode::COND; }
    ;

/* A value needs quoting only when it actually requires it (to hold ';', '#',
 * a quote char, or exact/multi-line whitespace); otherwise a bare run of
 * text works just as well. */
value:
    quoted_string { $$ = $1; }
    | BARE_STRING { $$ = $1; }
    ;

attrs:
    %empty      { $$ = noise::BindingAttr::NONE; }
    | attrs REQUIRED { $$ = $1 | noise::BindingAttr::REQUIRED; }
    | attrs KEEP_LEFT_WS { $$ = $1 | noise::BindingAttr::KEEP_LEFT_WS; }
    | attrs KEEP_RIGHT_WS { $$ = $1 | noise::BindingAttr::KEEP_RIGHT_WS; }
    | attrs KEEP_ENCLOSING_WS {
          $$ = $1 | noise::BindingAttr::KEEP_LEFT_WS |
               noise::BindingAttr::KEEP_RIGHT_WS;
      }
    | attrs QUOTED   { $$ = $1 | noise::BindingAttr::QUOTED; }
    | attrs BOOL     { $$ = $1 | noise::BindingAttr::BOOL; }
    ;

%%
