// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <Macro.h>

////////////////////////////////////////////////////////////////////////////////

namespace noise {

class ContextManager;

class UniformDistMacro : public Macro {
  public:
    UniformDistMacro(NoiseFlow* owner);
    ~UniformDistMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class UniformDistMacro

class ExponentialDistMacro : public Macro {
  public:
    ExponentialDistMacro(NoiseFlow* owner);
    ~ExponentialDistMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class ExponentialDistMacro

class NormalDistMacro : public Macro {
  public:
    NormalDistMacro(NoiseFlow* owner);
    ~NormalDistMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class NormalDistMacro

class BinomialDistMacro : public Macro {
  public:
    BinomialDistMacro(NoiseFlow* owner);
    ~BinomialDistMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class BinomialDistMacro

class PoissonDistMacro : public Macro {
  public:
    PoissonDistMacro(NoiseFlow* owner);
    ~PoissonDistMacro() {}
    std::string expand(ContextManager* context_manager) override;
};  // class PoissonDistMacro

};  // namespace noise
