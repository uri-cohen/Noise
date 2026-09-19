// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <Macro.h>

////////////////////////////////////////////////////////////////////////////////

namespace noise {

class ContextManager;

class UniformIntMacro : public Macro {
  public:
    UniformIntMacro(NoiseFlow* owner);
    ~UniformIntMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class UniformIntMacro

};  // namespace noise
