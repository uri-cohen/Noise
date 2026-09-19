// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// Context and ContextManager are the runtime book keepers for string values

#pragma once

#include <noise_std_include.h>
#include <optional>
#include <string>

namespace noise {

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

  private:
    std::map<std::string, std::string> _mapper;
    std::map<std::string, std::vector<std::string>> _shadows;
    std::vector<Context> _contexts;
};

}; // namespace noise
