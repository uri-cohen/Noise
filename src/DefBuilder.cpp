// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// the following block ties together the C-like yyextra
// with a within namespace object.
// bypassing the issue of no hook to place user code in the
// flex-generated h file
namespace noise {
struct DefLexerExtra;
}
using noise::DefLexerExtra;

#define YY_DECL noise::DefParser::symbol_type yylex(yyscan_t yyscanner)

#include <DefLexer.yy.hh>
#include <DefParser.tab.hh>

//

#include <NoiseFlow.h>
#include <DefBuilder.h>
#include <Exception.h>
#include <Logger.h>
#include <stdio.h>

using std::format;
using std::ifstream;
using std::string;

namespace noise {

DefLexerExtra::DefLexerExtra(NoiseFlow* o, const string& file_name)
    : owner(o),
      file_name(file_name),
      including_dir(std::filesystem::path(file_name).parent_path()),
      curr_text_set(false) {
    loc.initialize(&this->file_name);
}

DefBuilder::DefBuilder(NoiseFlow* owner, const std::string& file_name)
    : _file_name(file_name), _lexer_extra(owner, file_name) {}

int DefBuilder::parse() {
    FILE* in_file = fopen(std::filesystem::path(_file_name).c_str(), "r");
    if (!in_file) {
        throw NoiseIOError(format("failed opening {}", _file_name));
    }
    yyscan_t scanner;
    def_lex_init_extra(&_lexer_extra, &scanner);
    def_set_in(in_file, scanner);
    int res = -1;
    try {
        DefParser parser(scanner, &_lexer_extra);
        res = parser();
    } catch (const std::exception& e) {
        // Cleanup only: main()'s top-level catch is the single point that
        // logs FATAL for any propagated exception.
        def_lex_destroy(scanner);
        fclose(in_file);
        throw;
    }
    def_lex_destroy(scanner);
    fclose(in_file);
    return res;
}

}  // namespace noise
