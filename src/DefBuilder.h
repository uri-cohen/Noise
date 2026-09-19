// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <DefLexerExtra.h>
#include <noise_std_include.h>

namespace noise {

class NoiseFlow;

class DefBuilder {
  public:
    DefBuilder(NoiseFlow* owner, const std::string& file_name);
    virtual ~DefBuilder() {};
    int parse();

  private:
    std::string _file_name;
    DefLexerExtra _lexer_extra;
};  // DefBuilder

};  // namespace noise
