// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Context.h>
#include <Exception.h>
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

} // namespace noise
