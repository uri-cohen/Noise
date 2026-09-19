// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <noise_std_include.h>

namespace noise {

class NoiseFlow;
class ContextManager;
class Binding;

class Macro {
  public:
    Macro(const std::string& name);
    virtual ~Macro();

    virtual std::string expand(ContextManager* context_manager) = 0;

    void set_owner(NoiseFlow* owner) { _owner = owner; }
    NoiseFlow* owner() const { return _owner; }
    const std::string& name() const { return _name; }

    void add_param(const Binding& param);
    void add_arg(const Binding& arg);
    const std::vector<Binding>& params() const { return _params; }
    const std::vector<Binding>& args() const { return _args; }

    bool hasParams() const { return _hasParams; }
    bool hasArgs() const { return _hasArgs; }
    bool hasParams(bool v) { return _hasParams = v; }
    bool hasArgs(bool v) { return _hasArgs = v; }

    std::mt19937_64& gen();

  private:
    NoiseFlow* _owner;
    std::string _name;
    std::vector<Binding> _params;
    std::vector<Binding> _args;
    bool _hasParams;
    bool _hasArgs;
};

}; // namespace noise
