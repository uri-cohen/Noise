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
    for (const auto& [k, v]: context.locals())
    {
        if (_mapper.contains(k)) {
            _shadows[k].push_back(_mapper[k]);
        }
        _mapper[k] = v;
    }
    _contexts.push_back(context);
    return _contexts.back();
}

Context ContextManager::pop()
{
    if (_contexts.empty())
        throw NoiseInternalError("pop() on empty contexts stack");
    Context context = std::move(_contexts.back());
    _contexts.pop_back();

    for (const auto& [k, v] : context.locals()) {
        if (_shadows.contains(k)) {
            _mapper[k] = _shadows[k].back();
            _shadows[k].pop_back();
            if (_shadows[k].empty())
                _shadows.erase(k);
        } else {
            _mapper.erase(k);
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
    if (!_mapper.contains(name))
        return std::nullopt;
    return _mapper.at(name);
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
