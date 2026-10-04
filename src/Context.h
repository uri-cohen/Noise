// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// Context and ContextManager are the runtime book keepers for string values

#pragma once

#include <noise_std_include.h>
#include <optional>
#include <string>

namespace noise {

/// Configuration params: ordinary (global or per scope) params with a
/// reserved "NOISE_" prefix, read by the flow itself to tune its behavior.
/// Being params, they can be set globally (-D on the command line) and
/// overridden in any inner scope (e.g. a macro's PARAMS).
namespace config {
// VARS solver: max solver checks spent probing one variable's random value
// (0 disables probing - the solver's own model is taken as is)
inline constexpr const char* MAX_PROBES = "NOISE_MAX_PROBES";
inline constexpr int64_t MAX_PROBES_DEFAULT = 128;
// VARS solver: a REAL variable is sampled within [-range, range]
inline constexpr const char* REAL_RANGE = "NOISE_REAL_RANGE";
inline constexpr double REAL_RANGE_DEFAULT = 1e9;
// macro expansion nesting (recursion) limit - far below the few thousands
// levels a default (8MB) stack holds
inline constexpr const char* MAX_DEPTH = "NOISE_MAX_DEPTH";
inline constexpr int64_t MAX_DEPTH_DEFAULT = 256;
// max re-expansions of a macro's output (or a param's value) until it stops
// changing (Expansion Flow step 7)
inline constexpr const char* MAX_EXPANSIONS = "NOISE_MAX_EXPANSIONS";
inline constexpr int64_t MAX_EXPANSIONS_DEFAULT = 64;
// unknown $names reported at the end of a run in the warning itself; more
// than this many go to a details file the warning refers to
inline constexpr const char* MAX_UNKNOWN_WARNING = "NOISE_MAX_UNKNOWN_WARNING";
inline constexpr int64_t MAX_UNKNOWN_WARNING_DEFAULT = 10;
// max number of items a range macro may generate
inline constexpr const char* MAX_RANGE = "NOISE_MAX_RANGE";
inline constexpr int64_t MAX_RANGE_DEFAULT = 1024;
}  // namespace config

class Context {
  public:
    Context(std::istream* in_stream = nullptr,
            std::ostream* out_stream = nullptr,
            std::string* in_file_name = nullptr,
            uint32_t line = 0,
            uint32_t pos = 0,
            std::optional<std::map<std::string, std::string>> locals = std::nullopt)
    : _in_stream(in_stream)
    , _out_stream(out_stream)
    , _in_file_name(in_file_name)
    , _line(line)
    , _pos(pos)
    , _locals(locals.value_or(std::map<std::string,std::string>{})) {}
    Context(const Context& other) = default;
    Context(Context&& other) = default;
    ~Context() {}

    Context& operator=(Context& o) = default;
    Context& operator=(Context&& o) = default;

    std::istream* in_stream() const { return _in_stream; };
    std::ostream* out_stream() const { return _out_stream; };
    std::string* in_file_name() const { return _in_file_name; };
    uint32_t line() const { return _line; };
    uint32_t pos() const { return _pos; };

    uint32_t line(uint32_t v) { return _line = v; };
    uint32_t pos(uint32_t v) { return _pos = v; };
    
    void add_map(const std::string& name, const std::string& value) {
        _locals[name] = value;
    }

    const std::map<std::string, std::string>& locals() const { return _locals; }

  private:
    std::istream* _in_stream;
    std::ostream* _out_stream;
    std::string* _in_file_name;
    uint32_t _line;
    uint32_t _pos;

    std::map<std::string, std::string> _locals;
};

class ContextManager {
  public:
    ContextManager() {}
    ~ContextManager() {}

    Context& push(Context&& context);
    Context pop();
    Context& top();
    /// number of contexts (scope levels) - a context's level is its index
    size_t depth() const { return _contexts.size(); }

    /// A name's visible value: from the innermost scope that maps it.
    std::optional<const std::string> get(const std::string& name);
    /// As get(), but ignoring the innermost scope - e.g. for a builtin
    /// looking past its own invocation's context.
    std::optional<const std::string> get_outer(const std::string& name);
    /// Maps name in the innermost scope (adding or replacing its value there).
    void set_in_top(const std::string& name, const std::string& value);

    /// The next macro invocation id (#id): 1, 2, 3, ... over the whole run.
    uint64_t next_id() { return ++_last_id; }

    /// A configuration param's current value (see namespace config), or
    /// dflt if not set. Throws NoiseValueError if it is set to a value that
    /// is not a number, or is below min (for a real: not above min).
    int64_t config_int(const char* name, int64_t dflt, int64_t min);
    double config_real(const char* name, double dflt, double min);

  private:
    struct Entry {
        size_t level;  // index of the context in _contexts
        std::string value;
    };
    // per name, its entries in increasing level order; back() is the
    // visible one. Kept in sync with the contexts' locals (pop() removes
    // the entries of the names its context maps).
    std::map<std::string, std::vector<Entry>> _names;
    std::vector<Context> _contexts;
    uint64_t _last_id = 0;
};

}; // namespace noise
