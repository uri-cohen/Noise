// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <random>
//
#include <Binding.h>
#include <Context.h>
#include <Exception.h>
#include <NumericMacro.h>

using std::string;

namespace noise {

UniformDistMacro::UniformDistMacro(NoiseFlow* owner)
    : Macro("uniform_dist") {
    set_owner(owner);
    add_param(Binding("a", "0"));
    add_param(Binding("b", "1"));
    add_param(Binding("fmt", ""));
};

string UniformDistMacro::expand(ContextManager* context_manager) {
    double a = std::stod(*context_manager->get("a"));
    double b = std::stod(*context_manager->get("b"));
    string fmt = *context_manager->get("fmt");
    if (a > b) {
        throw NoiseValueError(
            std::format("uniform_dist: a ({}) must be <= b ({})", a, b));
    }
    std::uniform_real_distribution<double> d(a, b);
    double v = d(gen());
    return std::vformat("{:" + fmt + "}", std::make_format_args(v));
}

ExponentialDistMacro::ExponentialDistMacro(NoiseFlow* owner)
    : Macro("exp_dist") {
    set_owner(owner);
    add_param(Binding("lambda", "1"));
    add_param(Binding("fmt", ""));
};

string ExponentialDistMacro::expand(ContextManager* context_manager) {
    double lambda = std::stod(*context_manager->get("lambda"));
    string fmt = *context_manager->get("fmt");
    if (lambda <= 0) {
        throw NoiseValueError(
            std::format("exp_dist: lambda ({}) must be > 0", lambda));
    }
    std::exponential_distribution<double> d(lambda);
    double v = d(gen());
    return std::vformat("{:" + fmt + "}", std::make_format_args(v));
}

NormalDistMacro::NormalDistMacro(NoiseFlow* owner)
    : Macro("normal_dist") {
    set_owner(owner);
    add_param(Binding("mean", "0"));
    add_param(Binding("stddev", "1"));
    add_param(Binding("fmt", ""));
};

string NormalDistMacro::expand(ContextManager* context_manager) {
    double mean = std::stod(*context_manager->get("mean"));
    double stddev = std::stod(*context_manager->get("stddev"));
    string fmt = *context_manager->get("fmt");
    if (stddev <= 0) {
        throw NoiseValueError(
            std::format("normal_dist: stddev ({}) must be > 0", stddev));
    }
    std::normal_distribution<double> d(mean, stddev);
    double v = d(gen());
    return std::vformat("{:" + fmt + "}", std::make_format_args(v));
}

BinomialDistMacro::BinomialDistMacro(NoiseFlow* owner)
    : Macro("binomial_dist") {
    set_owner(owner);
    add_param(Binding("t", "1"));
    add_param(Binding("p", "0.5"));
    add_param(Binding("fmt", ""));
};

string BinomialDistMacro::expand(ContextManager* context_manager) {
    int64_t t = std::stoll(*context_manager->get("t"));
    double p = std::stod(*context_manager->get("p"));
    string fmt = *context_manager->get("fmt");
    if (t < 0) {
        throw NoiseValueError(
            std::format("binomial_dist: t ({}) must be >= 0", t));
    }
    if (p < 0 || p > 1) {
        throw NoiseValueError(
            std::format("binomial_dist: p ({}) must be in [0,1]", p));
    }
    std::binomial_distribution<int64_t> d(t, p);
    int64_t v = d(gen());
    return std::vformat("{:" + fmt + "}", std::make_format_args(v));
}

PoissonDistMacro::PoissonDistMacro(NoiseFlow* owner)
    : Macro("poisson_dist") {
    set_owner(owner);
    add_param(Binding("mean", "1"));
    add_param(Binding("fmt", ""));
};

string PoissonDistMacro::expand(ContextManager* context_manager) {
    double mean = std::stod(*context_manager->get("mean"));
    string fmt = *context_manager->get("fmt");
    if (mean <= 0) {
        throw NoiseValueError(
            std::format("poisson_dist: mean ({}) must be > 0", mean));
    }
    std::poisson_distribution<int64_t> d(mean);
    int64_t v = d(gen());
    return std::vformat("{:" + fmt + "}", std::make_format_args(v));
}

};  // namespace noise
