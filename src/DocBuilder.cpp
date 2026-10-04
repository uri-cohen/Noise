// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// the following block ties together the C-like yyextra
// with a within namespace object.
// bypassing the issue of no hook to place user code in the
// flex-generated h file
namespace noise {
struct DocLexerExtra;
}
using noise::DocLexerExtra;

#define YY_DECL noise::DocParser::symbol_type yylex(yyscan_t yyscanner)

#include <DocLexer.yy.hh>
#include <DocParser.tab.hh>

//

#include <Binding.h>
#include <Context.h>
#include <DocBuilder.h>
#include <Exception.h>
#include <Macro.h>
#include <NoiseFlow.h>

using std::string;

namespace noise {

DocLexerExtra::DocLexerExtra(NoiseFlow* o, const string& stream_id, std::ostream* out)
    : owner(o), stream_id(stream_id), out(out), out_line(1), out_col(1) {
    loc.initialize(&this->stream_id);
}

bool DocLexerExtra::is_macro(const std::string& id) const {
    return owner->is_expandable(id);
}

std::optional<std::string> DocLexerExtra::context_value(const std::string& id) const {
    return owner->context_manager()->get(id);
}

void DocLexerExtra::unknown_name(const std::string& name) const {
    owner->record_unknown(name, std::format("{}:{}.{}", stream_id, loc.begin.line,
                                            loc.begin.column));
}

bool DocLexerExtra::is_bool_formal(const std::string& id) const {
    Macro* macro = owner->find_macro(pending_macro_name);
    if (!macro) {
        return false;
    }
    for (const auto& formal : macro->params()) {
        if (formal.name() == id) {
            return formal.is_bool();
        }
    }
    for (const auto& formal : macro->args()) {
        if (formal.name() == id) {
            return formal.is_bool();
        }
    }
    return false;
}

void DocLexerExtra::emit(const std::string& text) {
    *out << text;
    for (char c : text) {
        if (c == '\n') {
            out_line++;
            out_col = 1;
        } else {
            out_col++;
        }
    }
}

DocBuilder::DocBuilder(NoiseFlow* owner, const std::string& stream_id, std::ostream& out)
    : _lexer_extra(owner, stream_id, &out) {}

int DocBuilder::parse(const std::string& input_text) {
    yyscan_t scanner;
    _lexer_extra.text_size = input_text.size();
    doc_lex_init_extra(&_lexer_extra, &scanner);
    YY_BUFFER_STATE buf = doc__scan_string(input_text.c_str(), scanner);
    int res = -1;
    try {
        DocParser parser(scanner, &_lexer_extra);
        res = parser();
    } catch (const std::exception& e) {
        // Cleanup only: main()'s top-level catch is the single point that
        // logs FATAL for any propagated exception.
        doc__delete_buffer(buf, scanner);
        doc_lex_destroy(scanner);
        throw;
    }
    doc__delete_buffer(buf, scanner);
    doc_lex_destroy(scanner);
    return res;
}

}  // namespace noise
