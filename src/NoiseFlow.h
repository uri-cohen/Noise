// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <noise_std_include.h>

#include <string>

namespace noise {

class Macro;
class ContextManager;
class VarBlock;
class VarSolver;

/// What an escaped "\$" becomes: a character no expansion round reads as the
/// start of a $name - a macro's output is re-expanded until stable, which
/// would otherwise turn a literal "$name" back into a name - and that only
/// the final output turns back into '$'.
inline constexpr char LITERAL_DOLLAR = '\x1F';

class NoiseFlow {
  public:
    NoiseFlow(uint64_t seed = 0);
    virtual ~NoiseFlow();

    void seed(uint64_t s) { _gen.seed(s); }

    int import(const std::filesystem::path& def_file_name);
    int import_include(const std::string& file_name,
                        const std::filesystem::path& including_dir);
    void add_include_dir(const std::filesystem::path& dir);

    int expand(std::istream& in, const std::string& top, std::ostream& out);
    int expand(const std::string& str, const std::string& top, std::ostream& out);
    int stream_expand(const std::string& text, const std::string& stream_id,
                       std::ostream& out);
    void add_macro(Macro* macro);
    Macro* find_macro(const std::string& id) const;
    std::mt19937_64& gen() { return _gen; }
    ContextManager* context_manager() const { return _context_manager; }
    VarSolver* var_solver() const { return _var_solver; }

    bool is_expandable(const std::string& id) const;

    /// current macro expansion nesting depth (see invoke_macro in
    /// DocParser.yy and config::MAX_DEPTH)
    int64_t& depth() { return _depth; }

    /// A top level (global scope) VARS clause. Resolved lazily, once, by
    /// the first top level expand() - not at import time, as the seed may
    /// only be set by a later command line option.
    void add_global_vars(std::shared_ptr<const VarBlock> vars);

    /// A global param (e.g. from the command line's --define): name -> value,
    /// visible to every scope below it (the document, macro bodies, VARS
    /// constraints). A later definition of the same name shadows it.
    void define_param(const std::string& name, const std::string& value);

    /// Pushes the process environment (variables whose name is an
    /// identifier) as a base context - below every -D param and scope.
    void import_environment();

    /// An unknown $name, expanded to nothing; `where` is "stream:line.col".
    void record_unknown(const std::string& name, const std::string& where);
    /// Issues the deferred warning about the unknown $names (if any): a
    /// summary, or - past NOISE_MAX_UNKNOWN_WARNING cases - a reference to a
    /// details file, written into details_dir.
    void report_unknowns(const std::filesystem::path& details_dir);
    void resolve_globals();

  private:
    ContextManager* _context_manager;
    VarSolver* _var_solver;
    std::map<std::string,Macro*> _macros;
    std::mt19937_64 _gen;
    std::vector<std::filesystem::path> _include_dirs;
    std::set<std::filesystem::path> _importing;
    std::vector<std::shared_ptr<const VarBlock>> _global_vars;
    bool _globals_resolved = false;
    int64_t _depth = 0;
    // unknown $names: (name, where), in order of occurrence
    std::vector<std::pair<std::string, std::string>> _unknowns;
};

}; // namespace noise
