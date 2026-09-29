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

    std::optional<const std::string> get(const std::string& name);

    /// A configuration param's current value (see namespace config), or
    /// dflt if not set. Throws NoiseValueError if it is set to a value that
    /// is not a number, or is below min (for a real: not above min).
    int64_t config_int(const char* name, int64_t dflt, int64_t min);
    double config_real(const char* name, double dflt, double min);

  private:
    std::map<std::string, std::string> _mapper;
    std::map<std::string, std::vector<std::string>> _shadows;
    std::vector<Context> _contexts;
};

}; // namespace noise
