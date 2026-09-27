// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <NoiseFlow.h>
#include <Macro.h>
#include <Binding.h>
#include <Context.h>
#include <DefBuilder.h>
#include <Exception.h>
#include <NumericMacro.h>
#include <TextMacro.h>

using std::format;

namespace noise {

NoiseFlow::NoiseFlow(uint64_t seed)
    : _context_manager(new ContextManager()), _gen(seed) {
    add_macro(new UniformDistMacro(this));
    add_macro(new ExponentialDistMacro(this));
    add_macro(new NormalDistMacro(this));
    add_macro(new BinomialDistMacro(this));
    add_macro(new PoissonDistMacro(this));
    add_macro(new RepeatMacro(this));
    add_macro(new SelectMacro(this));
    add_macro(new ForeachMacro(this));
}

NoiseFlow::~NoiseFlow() {
    delete _context_manager;
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

} // namespace noise
