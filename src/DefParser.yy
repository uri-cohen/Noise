/* clang-format off */

%code requires {

#include <NoiseFlow.h>
#include <TextMacro.h>
#include <Binding.h>
#include <Logger.h>

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
}

void end_macro(noise::DefLexerExtra* extra) {
    if (!extra->curr_text_set) {
        throw noise::NoiseDefBuilderError(
            std::format("macro '{}' missing TEXT", extra->curr_name));
    }
    auto* m = new noise::TextMacro(extra->curr_name, extra->curr_text);
    for (auto& p : extra->curr_params) {
        m->add_param(p);
    }
    for (auto& a : extra->curr_args) {
        m->add_arg(a);
    }
    extra->macro_list.push_back(m);
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

%start def_file

%%

def_file: defs END_OF_FILE { finalize(extra); } ;

defs:
    %empty
    | defs include
    | defs macro
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
    | param_list attrs ID '=' {
          def_push_DEF_VALUE_STATE(yyscanner);
      } value ';' {
          def_pop_state(yyscanner);
          extra->curr_params.emplace_back($3, $6, $2);
      }
    ;

arg_list:
    %empty
    | arg_list attrs ID '=' {
          def_push_DEF_VALUE_STATE(yyscanner);
      } value ';' {
          def_pop_state(yyscanner);
          extra->curr_args.emplace_back($3, $6, $2);
      }
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
