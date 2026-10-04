// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Cmd.h>
#include <noise_std_include.h>
#include <iostream>

using std::cout;
using std::endl;
using std::format;
using std::string;

////////////////////////////////////////////////////////////////////////////////

template <>
std::string parse_value<std::string>(const std::string& v) {
    return v;
}

template <>
bool parse_value<bool>(const std::string& v) {
    std::regex t_re = std::regex("true|True|TRUE|[tT]|on|On|ON|yes|Yes|YES|1");
    if (std::regex_match(v, t_re)) {
        return true;
    }
    std::regex f_re =
        std::regex("false|False|FALSE|[fF]|oof|Off|Off|no|No|NO|0");
    if (std::regex_match(v, t_re)) {
        return false;
    }
    throw CmdError(std::format("unknown bool value '{}'", v));
    return false;
}

template <>
long parse_value<long>(const std::string& v) {
    return std::stoll(v);
}

////////////////////////////////////////////////////////////////////////////////

Cmd::~Cmd() {
    for (auto o : _opts) {
        delete o;
    }
}

int Cmd::print_help(int i) {
    cout << _help << endl;
    for (auto o : _opts) {
        const string& h = o->help();
        cout << "    " << (o->code() ? "-" : "")
             << (o->code() ? format("{}", o->code()) : "")
             << ((o->code() && !o->name().empty()) ? ", " : "")
             << (o->name().empty() ? "" : "--")
             << (o->name().empty() ? "" : o->name()) << endl;
        int pos = 0, next_pos;
        while ((next_pos = h.find('\n', pos)) != std::string::npos) {
            cout << "         " << h.substr(pos, next_pos) << endl;
            pos = next_pos + 1;
        }
        cout << "         " << h.substr(pos) << endl;
    }
    exit(0);
    return 1;
}
int Cmd::print_version(int i) {
    cout << _version << endl;
    exit(0);
    return 1;
}

void Cmd::run() {
    auto process_non_opt = [&](const string& a) -> void {
        _non_opts.push_back(a);
        int inc = _non_opt_cb(this, -1);
        if (inc < 1) {
            throw CmdError(format("argv indexing stucked at {}", _current));
        }
        _current += inc;
    };

    pre_run();

    int& i = _current;
    for (i = 1; i < _argc;) {
        string a(_argv[i]);
        if (a.empty() || a[0] != '-') {
            process_non_opt(a);
            continue;
        }
        if (a == "--") {
            ++i;
            break;
        }
        bool is_long = (a.size() > 1 && a[1] == '-');
        string opt_name = a.substr(is_long ? 2 : 1);
        int v_pos = opt_name.find('=');
        if (!is_long && v_pos != string::npos) {
            throw CmdError(format(
                "'=' is not support for short option code ('{}')", _argv[i]));
        }
        if (v_pos != string::npos) {
            opt_name = opt_name.substr(0, v_pos);
        }
        if (!_arg2opt.contains(opt_name)) {
            throw CmdError(format("unknown option '{}'", _argv[i]));
        }

        CmdOptBase* opt = _arg2opt[opt_name];
        CmdOptCallback cb = opt->callback();
        int inc = cb(this, opt->index());
        if (inc < 1) {
            throw CmdError(format("argv indexing stucked at argv index {}", i));
        }
        i += inc;
    }
    for (; i < _argc;) {
        process_non_opt(_argv[i]);
    }

    post_run();
}
