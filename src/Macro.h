// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <noise_std_include.h>

namespace noise {

class NoiseFlow;
class ContextManager;
class Binding;
class VarBlock;

/// One EXPORT entry: maps `name` (to `value`, expanded in the exporting call;
/// else to the name's current value, or "true") in an outer scope - the
/// caller's, the document's, or that of the innermost calling macro named in
/// `macros` (skipped if none is calling).
struct Export {
    enum class Scope { CALLER, GLOBAL, MACROS };
    std::string name;
    Scope scope = Scope::CALLER;
    std::vector<std::string> macros;
    std::optional<std::string> value;
};

class Macro {
  public:
    Macro(const std::string& name);
    virtual ~Macro();

    virtual std::string expand(ContextManager* context_manager) = 0;
    /// builtins don't count as callers for EXPORT (see ContextManager)
    virtual bool is_builtin() const { return true; }

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

    /// The macro's VARS clause, resolved right after its params are bound
    /// (see invoke_macro in DocParser.yy); nullptr if it has none.
    const std::shared_ptr<const VarBlock>& vars() const { return _vars; }
    void set_vars(std::shared_ptr<const VarBlock> vars) { _vars = vars; }

    /// Throws NoiseDefBuilderError on a name exported twice.
    void add_export(const Export& e);
    const std::vector<Export>& exports() const { return _exports; }

  private:
    NoiseFlow* _owner;
    std::string _name;
    std::vector<Binding> _params;
    std::vector<Binding> _args;
    bool _hasParams;
    bool _hasArgs;
    std::shared_ptr<const VarBlock> _vars;
    std::vector<Export> _exports;
};

}; // namespace noise
