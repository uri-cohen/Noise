// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <NoiseFlow.h>
#include <Macro.h>
#include <Binding.h>
#include <Context.h>
#include <DefBuilder.h>
#include <Exception.h>
#include <NumericMacro.h>
#include <TextMacro.h>
#include <VarSolver.h>
#include <Vars.h>
#include <Logger.h>

extern char** environ;

using std::format;

namespace noise {

NoiseFlow::NoiseFlow(uint64_t seed)
    : _context_manager(new ContextManager()), _var_solver(new VarSolver()), _gen(seed) {
    add_macro(new UniformDistMacro(this));
    add_macro(new ExponentialDistMacro(this));
    add_macro(new NormalDistMacro(this));
    add_macro(new BinomialDistMacro(this));
    add_macro(new PoissonDistMacro(this));
    add_macro(new RepeatMacro(this));
    add_macro(new SelectMacro(this));
    add_macro(new ForeachMacro(this));
    add_macro(new SubstrMacro(this));
    add_macro(new ConcatMacro(this));
    add_macro(new IteMacro(this));
    add_macro(new RangeMacro(this));
}

NoiseFlow::~NoiseFlow() {
    delete _context_manager;
    delete _var_solver;
    for (auto& [name, macro] : _macros) {
        delete macro;
    }
}

int NoiseFlow::import(const std::filesystem::path& def_file_name)
{
    auto canon = std::filesystem::weakly_canonical(def_file_name);
    if (_importing.contains(canon)) {
        throw NoiseDefBuilderError(
            format("circular INCLUDE of '{}'", canon.string()));
    }
    _importing.insert(canon);
    DefBuilder builder(this, canon.string());
    int rc = builder.parse();
    _importing.erase(canon);
    return rc;
}

int NoiseFlow::import_include(const std::string& file_name,
                               const std::filesystem::path& including_dir)
{
    std::filesystem::path candidate = including_dir / file_name;
    if (!std::filesystem::exists(candidate)) {
        candidate.clear();
        for (const auto& dir : _include_dirs) {
            std::filesystem::path try_path = dir / file_name;
            if (std::filesystem::exists(try_path)) {
                candidate = try_path;
                break;
            }
        }
    }
    if (candidate.empty()) {
        throw NoiseIOError(
            format("INCLUDE '{}' not found in search path", file_name));
    }
    return import(candidate);
}

void NoiseFlow::add_include_dir(const std::filesystem::path& dir)
{
    std::string s = dir.string();
    std::string::size_type start = 0;
    while (start <= s.size()) {
        std::string::size_type sep = s.find(':', start);
        std::string entry = s.substr(start, sep == std::string::npos
                                                 ? std::string::npos
                                                 : sep - start);
        if (!entry.empty()) {
            _include_dirs.push_back(entry);
        }
        if (sep == std::string::npos) {
            break;
        }
        start = sep + 1;
    }
}

void NoiseFlow::add_macro(Macro* macro) { _macros[macro->name()] = macro; }

Macro* NoiseFlow::find_macro(const std::string& id) const
{
    auto it = _macros.find(id);
    return it == _macros.end() ? nullptr : it->second;
}

bool NoiseFlow::is_expandable(const std::string& id) const
{
    return _macros.contains(id);
}

void NoiseFlow::add_global_vars(std::shared_ptr<const VarBlock> vars)
{
    _global_vars.push_back(vars);
}

void NoiseFlow::define_param(const std::string& name, const std::string& value)
{
    static const std::regex ID_RE("[a-zA-Z][a-zA-Z0-9_]*");
    if (!std::regex_match(name, ID_RE)) {
        throw NoiseValueError(
            format("invalid global param name '{}' (expected an identifier)", name));
    }
    Context ctx;
    ctx.add_map(name, value);
    _context_manager->push(std::move(ctx));
}

void NoiseFlow::import_environment()
{
    static const std::regex ID_RE("[a-zA-Z][a-zA-Z0-9_]*");
    Context ctx;
    for (char** e = environ; e && *e; ++e) {
        std::string entry(*e);
        auto eq = entry.find('=');
        std::string name = entry.substr(0, eq);
        if (eq != std::string::npos && std::regex_match(name, ID_RE)) {
            ctx.add_map(name, entry.substr(eq + 1));
        }
    }
    _context_manager->push(std::move(ctx));
}

void NoiseFlow::record_unknown(const std::string& name, const std::string& where)
{
    _unknowns.emplace_back(name, where);
}

void NoiseFlow::report_unknowns(const std::filesystem::path& details_dir)
{
    if (_unknowns.empty()) {
        return;
    }
    // distinct names, in order of first occurrence, with their counts
    std::vector<std::string> names;
    std::map<std::string, size_t> counts;
    for (const auto& [name, where] : _unknowns) {
        if (counts[name]++ == 0) {
            names.push_back(name);
        }
    }
    size_t n = _unknowns.size();
    int64_t max = _context_manager->config_int(
        config::MAX_UNKNOWN_WARNING, config::MAX_UNKNOWN_WARNING_DEFAULT, 0);
    std::string head = format("{} unknown $name{} expanded to nothing", n,
                              n == 1 ? "" : "s");
    if (static_cast<int64_t>(n) <= max) {
        std::string list;
        for (const auto& name : names) {
            list += format("{}${}", list.empty() ? "" : ", ", name);
            if (counts[name] > 1) {
                list += format(" (x{})", counts[name]);
            }
        }
        WARNING(302, "{}: {}", head, list);
        return;
    }
    std::filesystem::path file = details_dir / "noise-unknown.txt";
    std::ofstream out(file);
    for (const auto& [name, where] : _unknowns) {
        out << where << " $" << name << "\n";
    }
    if (!out) {
        WARNING(303, "{} ({} distinct) - failed writing the details to '{}'", head,
                names.size(), file.string());
        return;
    }
    WARNING(304, "{} ({} distinct) - details in '{}'", head, names.size(),
            file.string());
}

void NoiseFlow::resolve_globals()
{
    if (_globals_resolved) {
        return;
    }
    _globals_resolved = true;
    // Each block becomes a permanent base context (never popped), in order,
    // so a later global block sees the earlier ones' variables as constants.
    for (const auto& block : _global_vars) {
        Context ctx;
        for (const auto& [name, value] :
             _var_solver->resolve(*block, _context_manager, _gen)) {
            ctx.add_map(name, value);
        }
        _context_manager->push(std::move(ctx));
    }
}

} // namespace noise
