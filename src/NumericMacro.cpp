// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <random>
//
#include <Binding.h>
#include <Context.h>
#include <Exception.h>
#include <NumExpr.h>
#include <NumericMacro.h>
#include <Vars.h>

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

// expr<str, fmt>: str - a param, so expanded already - evaluated as a numeric
// expression (see evaluate_num_expr); the int64, real or true/false result is
// rendered by fmt (a std::format spec, as for the <dist>_dist macros).
ExprMacro::ExprMacro(NoiseFlow* owner) : Macro("expr") {
    set_owner(owner);
    add_param(Binding("str", "", BindingAttr::REQUIRED));
    add_param(Binding("fmt", ""));
}

string ExprMacro::expand(ContextManager* context_manager) {
    string str = context_manager->get("str").value_or("");
    string fmt = context_manager->get("fmt").value_or("");
    NumValue v = evaluate_num_expr(str);
    string spec = "{:" + fmt + "}";
    try {
        switch (v.type) {
        case NumValue::Type::INT:
            return fmt.empty() ? std::to_string(v.i)
                               : std::vformat(spec, std::make_format_args(v.i));
        case NumValue::Type::REAL:
            return std::vformat(spec, std::make_format_args(v.d));
        case NumValue::Type::BOOL: {
            string b = v.b ? "true" : "false";
            return fmt.empty() ? b : std::vformat(spec, std::make_format_args(b));
        }
        }
    } catch (const std::format_error& e) {
        throw NoiseValueError(std::format("expr fmt '{}': {}", fmt, e.what()));
    }
    return "";
}

// min<params...> / max<params...>: the param of the least / greatest numeric
// value (int64 or real; the first one on a tie) - returned as written, so
// its own formatting is kept.
MinMaxMacro::MinMaxMacro(NoiseFlow* owner, bool is_min)
    : Macro(is_min ? "min" : "max"), _is_min(is_min) {
    set_owner(owner);
}

string MinMaxMacro::expand(ContextManager* context_manager) {
    long n = std::stol(context_manager->get("#params").value_or("0"));
    if (n == 0) {
        throw NoiseValueError(std::format("{} needs at least one param", name()));
    }
    string best_text;
    long double best = 0;
    for (long i = 0; i < n; ++i) {
        string text = context_manager->get(std::format("param[{}]", i)).value_or("");
        long double v;
        if (auto iv = parse_int64(text)) {
            v = static_cast<long double>(*iv);
        } else if (auto dv = parse_real(text)) {
            v = *dv;
        } else {
            throw NoiseValueError(
                std::format("{}: param {} '{}' is not a number", name(), i, text));
        }
        if (i == 0 || (_is_min ? v < best : v > best)) {
            best = v;
            best_text = text;
        }
    }
    return best_text;
}

};  // namespace noise
