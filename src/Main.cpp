// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Cmd.h>
#include <DefBuilder.h>
#include <Logger.h>
#include <NoiseFlow.h>
#include "Exception.h"

using noise::NoiseFlow;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::istream;
using std::ofstream;
using std::optional;
using std::ostream;
using std::regex;
using std::regex_match;
using std::string;
using std::filesystem::path;

////////////////////////////////////////////////////////////////////////////////

// NOISE_VERSION and NOISE_HELP_HEADER should (may) go to a
// separated header file.
static const std::string NOISE_VERSION("0.1.0");
static const std::string NOISE_HELP_HEADER(R"(NOISE
NAME
    noise - invoke the Non Deterministic Pre Processor

SYNOPSIS
    noise [[options] [files]]...

DESCRIPTION

OPTIONS
)");

////////////////////////////////////////////////////////////////////////////////

class NoiseCmd : public Cmd {
  public:
    NoiseCmd(int argc, char** argv);
    ~NoiseCmd() {}

    int process_in_file(int idx);
    int process_out_file(int idx);
    int process_def_file(int idx);
    int process_include_path(int idx);
    int process_seed(int idx);
    int process_define(int idx);
    int process_mode(int idx);
    int process_log_level(int idx);
    int process_log_file(int idx);

    virtual void pre_run() {}
    virtual void post_run() {
        if (_non_opts.size() == 0) {
            _out_p = _out_p ? _out_p : &cout;
            _noise_flow.expand(cin, _in_file_name, *_out_p);
        }
    }

  private:
    NoiseFlow _noise_flow;
    ostream* _out_p;
    istream* _in_p;
    string _in_file_name;
};

#define CB(M) [](Cmd* n, int i) { return dynamic_cast<NoiseCmd*>(n)->M(i); }

NoiseCmd::NoiseCmd(int argc, char** argv)
    : Cmd(argc, argv, CB(process_in_file), NOISE_VERSION, NOISE_HELP_HEADER),
      _in_p(nullptr),
      _out_p(&cout) {
    // clang-format off
    add<string>("out-file", 'o', string(""),
        "output file name (empty = stdout)",
        CB(process_out_file));
    add<string>("def", 'd', "",
        "def file name (must exists). May be repeated",
        CB(process_def_file));
    add<string>("include-path", 'I', "",
        "':'-separated search path for INCLUDE'd def files",
        CB(process_include_path));
    add<string>("define", 'D', "",
        "define a global param: <name>[=<value>] (value defaults to 'true').\n"
        "May be repeated; applies to the files that follow it",
        CB(process_define));
    add<long>("seed", 's', 0,
        "set seed (long int)",
        CB(process_seed));
    add<string>("mode", 'm', "file",
        "'file' or 'expr' - how to interpret non option args",
        CB(process_mode));
    add<bool>("file", 'f', true,
        "args are files to be pre-processed",
        CB(process_mode));
    add<bool>("expr", 'e', false,
        "args are string expressions to be pre-processed",
        CB(process_mode));
    add<string>("log-file", 'l', "",
        "log file name",
        CB(process_log_file));
    add<string>("log-level", '\0', "info",
        "either named level in { fatal, error, warning, info, debug }\n"
        "or a numeric value (10 to 50)",
        CB(process_log_level));
    // clang-format on
}

int NoiseCmd::process_in_file(int idx) {
    string a(_argv[_current]);
    optional<string> mode =
        dynamic_cast<CmdOpt<string>*>(_arg2opt["mode"])->value();
    _out_p = _out_p ? _out_p : &cout;
    if (mode && *mode == "expr") {
        return _noise_flow.expand(a, "<arg>", *_out_p);
    }
    _in_file_name = a;
    if (_in_p && _in_p != &cin) {
        dynamic_cast<ifstream*>(_in_p)->close();
        delete _in_p;
        _in_p = nullptr;
    }

    ifstream in_file;
    if (!_in_file_name.empty()) {
        in_file.open(path(_in_file_name));
        _in_p = &in_file;
    } else {
        _in_file_name = "<stdin>";
        _in_p = &cin;
    }
    if (!*_in_p) {
        _in_p = nullptr;
        throw noise::NoiseIOError(format(
            "expand_file failed openning input file '{}'", _in_file_name));
    }

    INFO(403, "setting input file to {}", _in_file_name);
    _noise_flow.expand(*_in_p, _in_file_name, *_out_p);
    return 1;
}

int NoiseCmd::process_out_file(int idx) {
    string out_file_name;
    int cap = check_arg("out-file", out_file_name);
    CmdOpt<string>* opt = dynamic_cast<CmdOpt<string>*>(_opts[idx]);
    ofstream out_file;
    ostream* _out_p = out_file_name.empty() ? &cout : &out_file;
    if (!out_file_name.empty()) {
        out_file.open(path(out_file_name));
    }
    if (!*_out_p) {
        _out_p = nullptr;
        opt->clear_value();
        throw noise::NoiseIOError(format(
            "expand_file failed openning output file '{}'", out_file_name));
    }
    opt->value(out_file_name);
    INFO(402, "setting output file to {}", out_file_name);
    return cap;
}

int NoiseCmd::process_def_file(int idx) {
    string def_file_name;
    int cap = check_arg("def", def_file_name);
    _noise_flow.import(def_file_name);
    return cap;
}

int NoiseCmd::process_include_path(int idx) {
    string path_arg;
    int cap = check_arg("include-path", path_arg);
    _noise_flow.add_include_dir(path_arg);
    return cap;
}

int NoiseCmd::process_seed(int idx) {
    long seed;
    int cap = check_arg("seed", seed);
    CmdOpt<long>* opt = dynamic_cast<CmdOpt<long>*>(_opts[idx]);
    opt->value(seed);
    _noise_flow.seed(static_cast<uint64_t>(seed));
    INFO(401, "setting seed to {}", seed);
    return cap;
}

int NoiseCmd::process_define(int idx) {
    string def;
    int cap = check_arg("define", def);
    auto eq = def.find('=');
    string name = def.substr(0, eq);
    string value = eq == string::npos ? string("true") : def.substr(eq + 1);
    _noise_flow.define_param(name, value);
    INFO(408, "defining global param {}={}", name, value);
    return cap;
}

int NoiseCmd::process_mode(int idx) {
    string mode(_argv[_current]);
    int cap = 1;
    if (mode == "--mode" || mode == "-m") {
        cap = check_arg("mode", mode);
    } else if (_opts[idx]->name() == "file") {
        mode = "file";
    } else {
        mode = "expr";
    }

    CmdOpt<string>* opt = dynamic_cast<CmdOpt<string>*>(_arg2opt["mode"]);
    opt->value(mode);
    INFO(404, "setting command line's non-opts interpretation mode to {}",
         mode);
    return cap;
}

int NoiseCmd::process_log_level(int idx) {
    uint32_t level = -1;
    string level_str;
    int cap = check_arg("log-level", level_str);
#define X(E, L, C, S)     \
    if (level_str == S) { \
        level = L;        \
    }
    PROJECT_LOG_LEVELS;
#undef X
    if (level == -1) {
        regex num_re("^[-+]?[0-9]+$");
        if (!regex_match(level_str, num_re)) {
            throw CmdError(format("bad log level '{}'", level_str));
        }
        level = std::stoul(level_str);
    }
    CmdOpt<string>* opt = dynamic_cast<CmdOpt<string>*>(_opts[idx]);
    opt->value(level_str);
    if (!Logger::logger) {
        Logger::init("", level);
        INFO(406, "start logging to <stderr>");
    } else {
        Logger::set_level(level);
    }
    INFO(405, "setting log level to {}", level_str);
    return cap;
}

int NoiseCmd::process_log_file(int idx) {
    string log_file;
    int cap = check_arg("log-file", log_file);

    CmdOpt<string>* opt = dynamic_cast<CmdOpt<string>*>(_opts[idx]);
    string curr_log_file = (opt->value() ? *(opt->value()) : "");

    if (!Logger::logger || log_file != curr_log_file) {
        INFO(407, "start logging to {}",
             log_file.empty() ? string("<stderr") : log_file);
        Logger::init(log_file, Logger::logger ? Logger::level()
                                              : int(Logger::Level::LOG_INFO));
    }
    opt->value(log_file);
    return cap;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    try {
        NoiseCmd cmd(argc, argv);
        cmd.run();
    } catch (std::exception& e) {
        string e_str(e.what());
        try {
            FATAL(101, "{}", e_str);
            return 1;
        } catch (std::exception& ee) {
            cerr << ee.what() << "\nwhile processing\n"
                 << "Noise-F-101: " << e_str << endl;
            return 2;
        }
    }
    return 0;
}
