// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <DocLexerExtra.h>
#include <noise_std_include.h>

namespace noise {

class NoiseFlow;

class DocBuilder {
  public:
    DocBuilder(NoiseFlow* owner, const std::string& stream_id, std::ostream& out);
    virtual ~DocBuilder() {};
    int parse(const std::string& input_text);

  private:
    DocLexerExtra _lexer_extra;
};  // DocBuilder

};  // namespace noise
