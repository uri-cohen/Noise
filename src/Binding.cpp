// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <Binding.h>
#include <Exception.h>

using std::format;

namespace noise {

void validate_binding_order(const std::vector<Binding>& list) {
    bool seen_optional = false;
    for (const auto& b : list) {
        if (b.required()) {
            if (seen_optional) {
                throw NoiseDefBuilderError(format(
                    "required entry '{}' follows a non-required entry",
                    b.name()));
            }
        } else {
            seen_optional = true;
        }
    }
}

};  // namespace noise
