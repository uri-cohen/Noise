// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Context.h>
#include <Exception.h>
#include <Utils.h>
#include <charconv>
#include <cmath>
#include <optional>

namespace noise {

Context& ContextManager::push(Context&& context)
{
    size_t level = _contexts.size();
    for (const auto& [k, v] : context.locals()) {
        _names[k].push_back(Entry{level, v});
    }
    _contexts.push_back(std::move(context));
    return _contexts.back();
}

Context ContextManager::pop()
{
    if (_contexts.empty())
        throw NoiseInternalError("pop() on empty contexts stack");
    size_t level = _contexts.size() - 1;
    Context context = std::move(_contexts.back());
    _contexts.pop_back();

    for (const auto& [k, v] : context.locals()) {
        auto it = _names.find(k);
        if (it == _names.end() || it->second.empty() || it->second.back().level != level) {
            throw NoiseInternalError(
                std::format("context stack out of sync popping '{}'", k));
        }
        it->second.pop_back();
        if (it->second.empty()) {
            _names.erase(it);
        }
    }
    return context;
}

Context& ContextManager::top()
{
    if (_contexts.empty())
        throw NoiseInternalError("top() on empty contexts stack");
    return _contexts.back();
}

std::optional<const std::string> ContextManager::get(const std::string& name)
{
    auto it = _names.find(name);
    if (it == _names.end()) {
        return std::nullopt;
    }
    return it->second.back().value;
}

std::optional<const std::string> ContextManager::get_outer(const std::string& name)
{
    auto it = _names.find(name);
    if (it == _names.end()) {
        return std::nullopt;
    }
    const auto& entries = it->second;
    size_t top = _contexts.size() - 1;
    for (auto e = entries.rbegin(); e != entries.rend(); ++e) {
        if (e->level < top) {
            return e->value;
        }
    }
    return std::nullopt;
}

void ContextManager::set_in_top(const std::string& name, const std::string& value)
{
    if (_contexts.empty())
        throw NoiseInternalError("set_in_top() on empty contexts stack");
    size_t level = _contexts.size() - 1;
    _contexts.back().add_map(name, value);
    auto& entries = _names[name];
    if (!entries.empty() && entries.back().level == level) {
        entries.back().value = value;
    } else {
        entries.push_back(Entry{level, value});
    }
}

int64_t ContextManager::config_int(const char* name, int64_t dflt, int64_t min)
{
    auto raw = get(name);
    if (!raw) {
        return dflt;
    }
    std::string v = trim(*raw);
    int64_t out = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (v.empty() || ec != std::errc() || ptr != v.data() + v.size() || out < min) {
        throw NoiseValueError(std::format(
            "config param {}='{}' must be an integer >= {}", name, *raw, min));
    }
    return out;
}

double ContextManager::config_real(const char* name, double dflt, double min)
{
    auto raw = get(name);
    if (!raw) {
        return dflt;
    }
    std::string v = trim(*raw);
    double out = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (v.empty() || ec != std::errc() || ptr != v.data() + v.size() ||
        !std::isfinite(out) || out <= min) {
        throw NoiseValueError(std::format(
            "config param {}='{}' must be a number > {}", name, *raw, min));
    }
    return out;
}

} // namespace noise
