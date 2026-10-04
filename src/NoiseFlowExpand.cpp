// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// Kept separate from NoiseFlow.cpp: DefLexerExtra.h and DocLexerExtra.h each
// pull in a Bison-generated `noise::location`/`noise::position` pair, and
// having both generated headers in one translation unit is an ODR clash
// (same class names, same namespace). DefBuilder-related NoiseFlow methods
// live in NoiseFlow.cpp; the doc-expansion ones live here.

#include <NoiseFlow.h>
#include <Exception.h>
#include <DocBuilder.h>
#include <algorithm>
#include <iterator>
#include <sstream>

namespace noise {

int NoiseFlow::expand(std::istream& in, const std::string& top, std::ostream& out)
{
    std::string text((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
    return expand(text, top, out);
}

int NoiseFlow::expand(const std::string& str, const std::string& top, std::ostream& out)
{
    resolve_globals();
    std::ostringstream expanded;
    int rc = stream_expand(str, top, expanded);
    // the final output: escaped "\$"s become '$' (see LITERAL_DOLLAR)
    std::string text = expanded.str();
    std::replace(text.begin(), text.end(), LITERAL_DOLLAR, '$');
    out << text;
    return rc;
}

int NoiseFlow::stream_expand(const std::string& text, const std::string& stream_id,
                              std::ostream& out)
{
    DocBuilder builder(this, stream_id, out);
    return builder.parse(text);
}

std::string NoiseFlow::expand_until_stable(const std::string& text,
                                           const std::string& stream_id,
                                           int64_t max_expansions)
{
    std::string prev = text, curr;
    int64_t iterations = 0;
    do {
        std::ostringstream tmp;
        stream_expand(prev, stream_id, tmp);
        curr = tmp.str();
        if (curr == prev) {
            break;
        }
        prev = curr;
    } while (++iterations < max_expansions);
    if (iterations >= max_expansions) {
        throw NoiseMacroCallError(std::format(
            "possible infinite expansion while expanding '{}' - not stable "
            "after NOISE_MAX_EXPANSIONS={} re-expansions", stream_id, max_expansions));
    }
    return prev;
}

} // namespace noise
