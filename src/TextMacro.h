// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <Macro.h>

////////////////////////////////////////////////////////////////////////////////

namespace noise {

class NoiseFlow;
class ContextManager;

class TextMacro : public Macro {
  public:
    TextMacro(const std::string& name, const std::string& text)
        : Macro(name), _text(text) {}
    ~TextMacro() {}
    std::string expand(ContextManager* context_manager) override;

  private:
    const std::string _text;

};  // class TextMacro

class RepeatMacro : public Macro {
  public:
    RepeatMacro(NoiseFlow* owner);
    ~RepeatMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class RepeatMacro

class SelectMacro : public Macro {
  public:
    SelectMacro(NoiseFlow* owner);
    ~SelectMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class SelectMacro

class ForeachMacro : public Macro {
  public:
    ForeachMacro(NoiseFlow* owner);
    ~ForeachMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class ForeachMacro

class SubstrMacro : public Macro {
  public:
    SubstrMacro(NoiseFlow* owner);
    ~SubstrMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class SubstrMacro

};  // namespace noise
