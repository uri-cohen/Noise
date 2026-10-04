// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// A general purpose argc/argv processor.
/// To be used as a base class for specific command class.
/// Note this is in global namespace...

#pragma once

#include <exception>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class CmdError : public std::exception {
  public:
    CmdError(const std::string& msg) : _msg(msg) {}
    virtual ~CmdError() {}
    virtual const char* what() const noexcept { return _msg.c_str(); }

  private:
    const std::string _msg;
};

template <typename T>
T parse_value(const std::string& arg);

/// CmdOptCallback is called per process argv item
/// with arguments: (Cmd, opt-index, value-pos).
/// * opt-index: in Cmd._opts
/// * pos-value: on --opt=value points to the =+1 position (otherwise 0)
/// It returns the number of argv items consumed by the callback.
/// The required/optional logic is within the callback.
class Cmd;
class CmdOptBase;
typedef std::function<int(Cmd*, int)> CmdOptCallback;

/// CmdOpt - A single command option
/// Must have a name and a default value

class CmdOptBase {
  public:
    CmdOptBase(const std::string& n, char c, const std::string& h, int i,
               CmdOptCallback cb)
        : _name(n), _short_code(c), _help(h), _count(0), _index(i), _cb(cb) {}
    virtual ~CmdOptBase() {}

    const std::string& name() const { return _name; }
    char code() const { return _short_code; }
    const std::string& help() const { return _help; }
    int count() const { return _count; }
    int index() const { return _index; }
    CmdOptCallback& callback() { return _cb; }

  private:
    std::string _name;   // (required) --`_name` is the long option code
    char _short_code;    // (optional) -`_short_code`
    std::string _help;   // --help text
    int _count;          // incremented when CmdOpt matches
    int _index;          // in Cmd._opts
    CmdOptCallback _cb;  // to be invoked per match
};

template <typename T>
class CmdOpt : public CmdOptBase {
  public:
    CmdOpt(const std::string& n, char c, T dv, const std::string& h, int i,
           CmdOptCallback cb)
        : CmdOptBase(n, c, h, i, cb), _default_value(dv), _value() {}
    virtual ~CmdOpt() {}

    virtual std::optional<T> value() const { return _value; }
    virtual std::optional<T> value(const T& v) {
        _value = v;
        return _value;
    }
    virtual void clear_value() { _value = std::nullopt; }
    virtual T default_value() const { return _default_value; }

  private:
    T _default_value;         // (required)
    std::optional<T> _value;  // (optional) to be initialized to default value
};

class Cmd {
  public:
    Cmd(int argc, char** argv, CmdOptCallback nocb, const std::string& v,
        const std::string help_header)
        : _argc(argc),
          _argv(argv),
          _non_opt_cb(nocb),
          _version(v),
          _help(help_header) {
        add("help", 'h', false, "print help message", &Cmd::print_help);
        add("version", 'v', false, "print version", &Cmd::print_version);
    }
    virtual ~Cmd();

    // the following are CmdOptCallback. This is why not const
    int print_help(int i = 0);
    int print_version(int i = 0);

    template <typename T>
    int check_arg(const std::string& opt_name, T& value, int offset = 1);

    template <typename T>
    void add(const std::string& n, char c, T dv, const std::string& h,
             CmdOptCallback cb);
    void run();

    virtual void pre_run() = 0;
    virtual void post_run() = 0;

  protected:
    int _argc;
    char** _argv;

    const std::string _help;
    const std::string _version;
    CmdOptCallback _non_opt_cb;

    std::vector<CmdOptBase*> _opts;
    int _current;  // current argv index being processed
    std::map<std::string, CmdOptBase*> _arg2opt;
    std::vector<std::string> _non_opts;
};

template <typename T>
int Cmd::check_arg(const std::string& opt_name, T& value, int offset) {
    std::string arg = std::string(_argv[_current]).substr(2);
    int pos = 0;
    if (arg.size() > opt_name.size()) {
        if (arg[opt_name.size()] != '=') {
            throw CmdError(
                format("Internal error in option '{}'. "
                       "expected '=' after it",
                       opt_name));
        }
        pos = opt_name.size() + 3;  // 3 = '--' + '='
        offset -= 1;
    }
    if (_argc <= _current + offset) {
        throw CmdError(format("not enough args for {}", opt_name));
    }
    std::string s = std::string(_argv[_current + offset]).substr(pos);
    value = parse_value<T>(s);
    return offset + 1;
}

template <typename T>
void Cmd::add(const std::string& n, char c, T dv, const std::string& h,
              CmdOptCallback cb) {
    if (_opts.size() && n.empty() && c == '\0') {
        throw CmdError("either short or long name must be specified");
    }
    if (_arg2opt.contains(n)) {
        throw CmdError(format("long name '{}' already in use", n));
    }
    std::string sc(std::format("{}", c));
    if (n == sc) {
        throw CmdError("long and short option codes cannot be equal");
    }
    if (_arg2opt.contains(sc)) {
        throw CmdError(std::format("short option code '{}' already in use", c));
    }
    CmdOpt<T>* o = new CmdOpt<T>(n, c, dv, h, _opts.size(), cb);
    _opts.push_back(o);
    if (!n.empty()) {
        _arg2opt[n] = o;
    }
    if (!sc.empty()) {
        _arg2opt[sc] = o;
    }
}
