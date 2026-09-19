// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <random>
//
#include <Binding.h>
#include <Context.h>
#include <NumericMacro.h>

using std::string;

namespace noise {

UniformIntMacro::UniformIntMacro(NoiseFlow* owner)
    : Macro("uniform_int") {
    set_owner(owner);
    add_param(Binding("a", "0"));
    add_param(Binding("b", "1"));
};

string UniformIntMacro::expand(ContextManager* context_manager) {
    long int a = std::stoll(*context_manager->get("a"));
    long int b = std::stoll(*context_manager->get("b"));
    std::uniform_int_distribution<int64_t> d(a, b);
    return std::format("{}", d(gen()));
}

};  // namespace noise
