// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <numeric>
#include <z3++.h>
//
#include <Context.h>
#include <Exception.h>
#include <VarSolver.h>
#include <Vars.h>

using std::format;
using std::string;

namespace noise {

namespace {

using Op = Expr::Op;

// A translated (sub)expression: either typed (a Z3 expr of a known VarType)
// or untyped (a literal/name text, adopting the type of the operand it meets).
struct Val {
    std::optional<VarType> type;
    std::optional<z3::expr> e;
    string text;

    bool typed() const { return type.has_value(); }
};

// A plain decimal is passed to Z3 as is (exact); anything else goes through
// a double, printed fixed-point as Z3 numerals have no exponent form.
string real_numeral(const string& text, double d) {
    static const std::regex PLAIN_DECIMAL(R"(\s*-?[0-9]+(\.[0-9]+)?\s*)");
    if (std::regex_match(text, PLAIN_DECIMAL)) {
        return string(text.begin() + text.find_first_not_of(" \t\n"),
                      text.begin() + text.find_last_not_of(" \t\n") + 1);
    }
    return format("{:.17f}", d);
}

class Resolver {
  public:
    // Config params (see namespace config in Context.h) are read here, as
    // seen from the scope the VARS block is resolved in.
    Resolver(z3::context& ctx, ContextManager* cm, std::mt19937_64& gen)
        : _ctx(ctx), _solver(ctx), _cm(cm), _gen(gen),
          _max_probes(cm ? cm->config_int(config::MAX_PROBES,
                                          config::MAX_PROBES_DEFAULT, 0)
                         : config::MAX_PROBES_DEFAULT),
          _real_range(cm ? cm->config_real(config::REAL_RANGE,
                                           config::REAL_RANGE_DEFAULT, 0)
                         : config::REAL_RANGE_DEFAULT) {
        z3::params p(_ctx);
        p.set("random_seed", static_cast<unsigned>(_gen() & 0xffffffffu));
        _solver.set(p);
        // A push up front puts the solver in its incremental mode right
        // away - cheaper than the non-incremental first check it otherwise
        // starts with (and then abandons once probing pushes).
        _solver.push();
    }

    std::map<string, string> resolve(const VarBlock& block) {
        for (const auto& item : block.items()) {
            if (auto* d = std::get_if<VarDecl>(&item)) {
                declare(*d);
            } else if (auto* a = std::get_if<VarAssert>(&item)) {
                VarType b{VarKind::BOOL};
                _solver.add(lift(translate(*a->expr, b), b, "ASSERT"));
            } else if (auto* s = std::get_if<VarSmtlib>(&item)) {
                add_smtlib(s->code);
            }
        }
        // STRING variables range over the literal table, complete by now.
        for (const auto& v : _vars) {
            if (v.decl.type.kind == VarKind::STRING) {
                _solver.add(v.e >= 0);
                _solver.add(v.e < static_cast<int>(_strings.size()));
            }
        }

        if (check() != z3::sat) {
            return fallbacks();
        }
        std::vector<size_t> order(_vars.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), _gen);
        for (size_t i : order) {
            probe(_vars[i]);
        }
        // Only re-check if the last check (a failed probe) left no model.
        if (!_model_valid && check() != z3::sat) {
            throw NoiseInternalError("VARS solver lost satisfiability while probing");
        }
        z3::model m = _solver.get_model();
        std::map<string, string> out;
        for (const auto& v : _vars) {
            out[v.decl.name] = render(v, m.eval(v.e, true));
        }
        return out;
    }

  private:
    struct Var {
        VarDecl decl;
        z3::expr e;
    };

    z3::context& _ctx;
    z3::solver _solver;
    // whether the last check was SAT, i.e. the solver holds a current model
    bool _model_valid = false;
    ContextManager* _cm;
    std::mt19937_64& _gen;
    // max solver checks spent on probing a single variable's random value
    const int64_t _max_probes;
    // initial sampling range of a REAL variable (it has no natural one)
    const double _real_range;

    std::vector<Var> _vars;
    std::map<string, size_t> _var_idx;
    std::vector<string> _strings;
    std::map<string, int> _string_idx;

    // ---------------------------------------------------------------- decls

    void declare(const VarDecl& d) {
        const char* n = d.name.c_str();
        std::optional<z3::expr> e;
        switch (d.type.kind) {
        case VarKind::BOOL: e = _ctx.bool_const(n); break;
        case VarKind::INT:
        case VarKind::UINT:
        case VarKind::BITVEC: e = _ctx.bv_const(n, d.type.bv_width()); break;
        case VarKind::REAL: e = _ctx.real_const(n); break;
        case VarKind::STRING:
            e = _ctx.int_const(n);
            if (d.fallback) {
                intern(*d.fallback);
            }
            break;
        }
        _var_idx[d.name] = _vars.size();
        _vars.push_back(Var{d, *e});
    }

    void add_smtlib(const string& code) {
        z3::sort_vector sorts(_ctx);
        z3::func_decl_vector decls(_ctx);
        for (const auto& v : _vars) {
            decls.push_back(v.e.decl());
        }
        z3::expr_vector asserts = _ctx.parse_string(code.c_str(), sorts, decls);
        for (unsigned i = 0; i < asserts.size(); ++i) {
            _solver.add(asserts[i]);
        }
    }

    int intern(const string& s) {
        auto it = _string_idx.find(s);
        if (it != _string_idx.end()) {
            return it->second;
        }
        int idx = static_cast<int>(_strings.size());
        _strings.push_back(s);
        _string_idx[s] = idx;
        return idx;
    }

    // --------------------------------------------------------------- typing

    static VarType infer(const string& text) {
        if (parse_int64(text)) {
            return VarType{VarKind::INT};
        }
        if (parse_real(text)) {
            return VarType{VarKind::REAL};
        }
        string v = text;
        for (auto& c : v) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        if (v == "true" || v == "false") {
            return VarType{VarKind::BOOL};
        }
        return VarType{VarKind::STRING};
    }

    static VarType combine(const VarType& a, const VarType& b) {
        if (a == b) {
            return a;
        }
        if (a.is_numeric() && b.is_numeric()) {
            return VarType{VarKind::REAL};
        }
        return VarType{VarKind::STRING};
    }

    string name_text(const Expr& x) const {
        return _cm ? _cm->get(x.text).value_or(x.text) : x.text;
    }

    // The type an expression has regardless of its context, or nullopt if
    // it has no typed (variable/cast) operand deciding it.
    std::optional<VarType> static_type(const Expr& x) const {
        switch (x.op) {
        case Op::LITERAL:
        case Op::NAME:
            return std::nullopt;
        case Op::VAR:
            return _vars.at(_var_idx.at(x.text)).decl.type;
        case Op::CAST:
            return x.type;
        case Op::NOT: case Op::AND: case Op::OR: case Op::XOR: case Op::IMPLIES:
        case Op::LT: case Op::LE: case Op::GT: case Op::GE: case Op::EQ: case Op::NE:
            return VarType{VarKind::BOOL};
        case Op::NEG:
        case Op::BITNOT:
            return static_type(*x.kids[0]);
        default:
            if (auto t = static_type(*x.kids[0])) {
                return t;
            }
            return static_type(*x.kids[1]);
        }
    }

    // Last resort typing of an all-untyped expression: by its leaves' text.
    VarType infer_tree(const Expr& x) const {
        switch (x.op) {
        case Op::LITERAL: return infer(x.text);
        case Op::NAME: return infer(name_text(x));
        default:
            if (x.kids.size() == 1) {
                return infer_tree(*x.kids[0]);
            }
            return combine(infer_tree(*x.kids[0]), infer_tree(*x.kids[1]));
        }
    }

    // Returns v as a Z3 expr of type t: a typed value must already be of type
    // t, an untyped one is parsed from its text as a t value.
    z3::expr lift(const Val& v, const VarType& t, const char* where) {
        if (v.typed()) {
            if (*v.type != t) {
                throw NoiseValueError(format(
                    "{} expects {}, got {} - an explicit cast is required",
                    where, t.str(), v.type->str()));
            }
            return *v.e;
        }
        auto bad = [&]() {
            return NoiseValueError(
                format("'{}' is not a valid {} value", v.text, t.str()));
        };
        switch (t.kind) {
        case VarKind::BOOL:
            if (auto b = parse_bool(v.text)) {
                return _ctx.bool_val(*b);
            }
            throw bad();
        case VarKind::INT:
            if (auto i = parse_int64(v.text)) {
                return _ctx.bv_val(static_cast<int64_t>(*i), 64);
            }
            throw bad();
        case VarKind::UINT:
            if (auto u = parse_uint64(v.text)) {
                return _ctx.bv_val(static_cast<uint64_t>(*u), 64);
            }
            throw bad();
        case VarKind::BITVEC:
            if (auto c = canonical_value(t, v.text)) {
                return _ctx.bv_val(c->c_str(), t.width);
            }
            throw bad();
        case VarKind::REAL:
            if (auto d = parse_real(v.text)) {
                return _ctx.real_val(real_numeral(v.text, *d).c_str());
            }
            throw bad();
        case VarKind::STRING:
            return _ctx.int_val(intern(v.text));
        }
        throw bad();
    }

    static Val typed(const VarType& t, const z3::expr& e) {
        return Val{t, e, ""};
    }

    // C-style cast "(T)x"
    Val convert(const Val& v, const VarType& t) {
        if (!v.typed()) {
            return typed(t, lift(v, t, "cast"));
        }
        const VarType& f = *v.type;
        const z3::expr& e = *v.e;
        if (f == t) {
            return v;
        }
        auto bad = [&]() {
            return NoiseValueError(
                format("unsupported cast from {} to {}", f.str(), t.str()));
        };
        if (t.kind == VarKind::BOOL) {
            switch (f.kind) {
            case VarKind::REAL: return typed(t, e != _ctx.real_val(0));
            case VarKind::STRING: return typed(t, e != _ctx.int_val(intern("")));
            default: return typed(t, e != _ctx.bv_val(0, f.bv_width()));
            }
        }
        if (f.kind == VarKind::BOOL) {
            if (t.is_bv()) {
                unsigned w = t.bv_width();
                return typed(t, z3::ite(e, _ctx.bv_val(1, w), _ctx.bv_val(0, w)));
            }
            if (t.kind == VarKind::REAL) {
                return typed(t, z3::ite(e, _ctx.real_val(1), _ctx.real_val(0)));
            }
            throw bad();
        }
        if (f.is_bv() && t.is_bv()) {
            unsigned wf = f.bv_width(), wt = t.bv_width();
            if (wt == wf) {
                return typed(t, e);
            }
            if (wt > wf) {
                return typed(t, f.kind == VarKind::INT ? z3::sext(e, wt - wf)
                                                       : z3::zext(e, wt - wf));
            }
            return typed(t, e.extract(wt - 1, 0));
        }
        if (f.is_bv() && t.kind == VarKind::REAL) {
            return typed(t, z3::to_real(z3::bv2int(e, f.kind == VarKind::INT)));
        }
        if (f.kind == VarKind::REAL && t.is_bv()) {
            z3::expr i(_ctx, Z3_mk_real2int(_ctx, e));
            return typed(t, z3::int2bv(t.bv_width(), i));
        }
        throw bad();
    }

    // ---------------------------------------------------------- translation

    // Typing is bidirectional: an operator's operand type is taken from a
    // typed operand if any (static_type), else from the context the
    // expression is used in (hint), and only else inferred from literal text.
    // So in "u >= limit - 1" (u a UINT) "limit - 1" is UINT arithmetic.
    Val translate(const Expr& x, const std::optional<VarType>& hint) {
        switch (x.op) {
        case Op::LITERAL:
        case Op::NAME: {
            Val v{std::nullopt, std::nullopt,
                  x.op == Op::NAME ? name_text(x) : x.text};
            if (hint) {
                return typed(*hint, lift(v, *hint, "value"));
            }
            return v;
        }
        case Op::VAR: {
            const Var& v = _vars.at(_var_idx.at(x.text));
            return typed(v.decl.type, v.e);
        }
        case Op::CAST: {
            const Expr& kid = *x.kids[0];
            return convert(translate(kid, static_type(kid) ? std::nullopt
                                                           : std::optional(x.type)),
                           x.type);
        }
        case Op::NEG: {
            auto t = static_type(x);
            Val a = translate(*x.kids[0], t ? t : hint);
            if (!a.typed()) {
                // stays untyped: negate the literal text itself
                string text = a.text;
                auto nz = text.find_first_not_of(" \t\n");
                if (nz != string::npos && text[nz] == '-') {
                    text.erase(nz, 1);
                } else {
                    text = "-" + text;
                }
                return Val{std::nullopt, std::nullopt, text};
            }
            require(a.type->is_numeric(), x.op, *a.type);
            return typed(*a.type, -*a.e);
        }
        case Op::BITNOT: {
            VarType t = operand_type(x, hint);
            require(t.is_bv(), x.op, t);
            return typed(t, ~lift(translate(*x.kids[0], t), t, op_name(x.op)));
        }
        case Op::NOT: {
            VarType b{VarKind::BOOL};
            return typed(b, !lift(translate(*x.kids[0], b), b, "NOT"));
        }
        case Op::AND:
        case Op::OR:
        case Op::XOR:
        case Op::IMPLIES: {
            VarType b{VarKind::BOOL};
            z3::expr l = lift(translate(*x.kids[0], b), b, op_name(x.op));
            z3::expr r = lift(translate(*x.kids[1], b), b, op_name(x.op));
            switch (x.op) {
            case Op::AND: return typed(b, l && r);
            case Op::OR: return typed(b, l || r);
            case Op::XOR: return typed(b, l ^ r);
            default: return typed(b, z3::implies(l, r));
            }
        }
        default:
            return binary(x, hint);
        }
    }

    VarType operand_type(const Expr& x, const std::optional<VarType>& hint) const {
        if (auto t = static_type(x)) {
            return *t;
        }
        return hint ? *hint : infer_tree(x);
    }

    static void require(bool ok, Op op, const VarType& t) {
        if (!ok) {
            throw NoiseValueError(
                format("operator '{}' not supported for {}", op_name(op), t.str()));
        }
    }

    Val binary(const Expr& x, const std::optional<VarType>& hint) {
        const Expr& a = *x.kids[0];
        const Expr& b = *x.kids[1];
        bool is_compare = x.op == Op::EQ || x.op == Op::NE || x.op == Op::LT ||
                          x.op == Op::LE || x.op == Op::GT || x.op == Op::GE;
        // A comparison's own type is BOOL; its operands' type comes from
        // themselves alone.
        VarType t;
        if (is_compare) {
            auto ta = static_type(a);
            auto tb = static_type(b);
            t = ta ? *ta : tb ? *tb : combine(infer_tree(a), infer_tree(b));
        } else {
            t = operand_type(x, hint);
        }
        z3::expr l = lift(translate(a, t), t, op_name(x.op));
        z3::expr r = lift(translate(b, t), t, op_name(x.op));
        VarType boolean{VarKind::BOOL};
        bool is_signed = t.kind == VarKind::INT;
        bool is_real = t.kind == VarKind::REAL;
        switch (x.op) {
        case Op::EQ: return typed(boolean, l == r);
        case Op::NE: return typed(boolean, l != r);
        case Op::ADD: require(t.is_numeric(), x.op, t); return typed(t, l + r);
        case Op::SUB: require(t.is_numeric(), x.op, t); return typed(t, l - r);
        case Op::MUL: require(t.is_numeric(), x.op, t); return typed(t, l * r);
        case Op::DIV:
            require(t.is_numeric(), x.op, t);
            return typed(t, (is_real || is_signed) ? l / r : z3::udiv(l, r));
        case Op::MOD:
            require(t.is_bv(), x.op, t);
            return typed(t, is_signed ? z3::srem(l, r) : z3::urem(l, r));
        case Op::SHL: require(t.is_bv(), x.op, t); return typed(t, z3::shl(l, r));
        case Op::SHR:
            require(t.is_bv(), x.op, t);
            return typed(t, is_signed ? z3::ashr(l, r) : z3::lshr(l, r));
        case Op::BITAND: require(t.is_bv(), x.op, t); return typed(t, l & r);
        case Op::BITOR: require(t.is_bv(), x.op, t); return typed(t, l | r);
        case Op::BITXOR: require(t.is_bv(), x.op, t); return typed(t, l ^ r);
        case Op::LT:
        case Op::LE:
        case Op::GT:
        case Op::GE: {
            require(t.is_numeric(), x.op, t);
            bool native = is_real || is_signed;  // z3's operators are signed on BVs
            switch (x.op) {
            case Op::LT: return typed(boolean, native ? l < r : z3::ult(l, r));
            case Op::LE: return typed(boolean, native ? l <= r : z3::ule(l, r));
            case Op::GT: return typed(boolean, native ? l > r : z3::ugt(l, r));
            default: return typed(boolean, native ? l >= r : z3::uge(l, r));
            }
        }
        default:
            throw NoiseInternalError(
                format("unexpected VARS operator '{}'", op_name(x.op)));
        }
    }

    // -------------------------------------------------------------- probing

    // Adds c to the solver if it keeps it satisfiable (the push is kept).
    z3::check_result check() {
        z3::check_result r = _solver.check();
        _model_valid = r == z3::sat;
        return r;
    }

    bool try_add(const z3::expr& c) {
        _solver.push();
        _solver.add(c);
        if (check() == z3::sat) {
            return true;
        }
        _solver.pop();
        return false;
    }

    // Random-point bisection over [lo, hi]: try v == c for a uniform c; if
    // that is infeasible keep the half (v >= c or v < c) holding solutions.
    // For a single feasible interval this is a uniform pick from it.
    template <typename T, typename Mk, typename Ge, typename Dist>
    void bisect(const z3::expr& v, T lo, T hi, Mk mk, Ge ge, Dist dist) {
        for (int64_t i = 0; i < _max_probes && lo <= hi; ++i) {
            T c = dist(lo, hi);
            z3::expr ce = mk(c);
            if (try_add(v == ce)) {
                return;
            }
            if (try_add(ge(v, ce))) {
                if constexpr (std::is_integral_v<T>) {
                    if (c == hi) return;
                    lo = c + 1;
                } else {
                    lo = c;
                }
            } else {
                if constexpr (std::is_integral_v<T>) {
                    if (c == lo) return;
                    hi = c - 1;
                } else {
                    hi = c;
                }
            }
        }
    }

    template <typename T>
    T uniform_int(T lo, T hi) {
        return std::uniform_int_distribution<T>(lo, hi)(_gen);
    }

    void probe(const Var& v) {
        if (_max_probes == 0) {
            return;  // probing disabled: the solver's own model is taken
        }
        const VarType& t = v.decl.type;
        switch (t.kind) {
        case VarKind::BOOL:
            try_add(v.e == _ctx.bool_val((_gen() & 1) != 0));
            return;
        case VarKind::STRING: {
            std::vector<int> idx(_strings.size());
            std::iota(idx.begin(), idx.end(), 0);
            std::shuffle(idx.begin(), idx.end(), _gen);
            if (static_cast<int64_t>(idx.size()) > _max_probes) {
                idx.resize(static_cast<size_t>(_max_probes));
            }
            for (int i : idx) {
                if (try_add(v.e == _ctx.int_val(i))) {
                    return;
                }
            }
            return;
        }
        case VarKind::INT:
            bisect<int64_t>(
                v.e, INT64_MIN, INT64_MAX,
                [&](int64_t c) { return _ctx.bv_val(c, 64); },
                [](const z3::expr& a, const z3::expr& b) { return a >= b; },
                [&](int64_t lo, int64_t hi) { return uniform_int(lo, hi); });
            return;
        case VarKind::UINT:
        case VarKind::BITVEC: {
            unsigned w = t.bv_width();
            if (w > 64) {
                return;  // no probing - the model's value is taken as is
            }
            uint64_t hi = w == 64 ? UINT64_MAX : ((uint64_t{1} << w) - 1);
            bisect<uint64_t>(
                v.e, 0, hi,
                [&](uint64_t c) { return _ctx.bv_val(c, w); },
                [](const z3::expr& a, const z3::expr& b) { return z3::uge(a, b); },
                [&](uint64_t lo, uint64_t hi) { return uniform_int(lo, hi); });
            return;
        }
        case VarKind::REAL:
            bisect<double>(
                v.e, -_real_range, _real_range,
                [&](double c) {
                    return _ctx.real_val(format("{:.17f}", c).c_str());
                },
                [](const z3::expr& a, const z3::expr& b) { return a >= b; },
                [&](double lo, double hi) {
                    return std::uniform_real_distribution<double>(lo, hi)(_gen);
                });
            return;
        }
    }

    // ------------------------------------------------------------ rendering

    string render(const Var& v, const z3::expr& val) {
        const VarType& t = v.decl.type;
        switch (t.kind) {
        case VarKind::BOOL:
            return val.is_true() ? "true" : "false";
        case VarKind::INT:
            return std::to_string(static_cast<int64_t>(val.get_numeral_uint64()));
        case VarKind::UINT:
            return std::to_string(val.get_numeral_uint64());
        case VarKind::BITVEC: {
            if (t.width <= 64) {
                return std::to_string(val.get_numeral_uint64());
            }
            string s;
            val.is_numeral(s);
            return s;
        }
        case VarKind::REAL: {
            string s = val.get_decimal_string(17);
            if (!s.empty() && s.back() == '?') {
                s.pop_back();
            }
            auto d = parse_real(s);
            return d ? format("{}", *d) : s;
        }
        case VarKind::STRING:
            return _strings.at(static_cast<size_t>(val.get_numeral_int()));
        }
        return "";
    }

    std::map<string, string> fallbacks() {
        std::map<string, string> out;
        string missing;
        for (const auto& v : _vars) {
            if (v.decl.fallback) {
                out[v.decl.name] = *v.decl.fallback;
            } else {
                missing += (missing.empty() ? "'" : ", '") + v.decl.name + "'";
            }
        }
        if (!missing.empty()) {
            throw NoiseValueError(format(
                "VARS constraints are unsatisfiable and variable(s) {} have no "
                "fallback value", missing));
        }
        return out;
    }
};

}  // namespace

struct VarSolver::Impl {
    z3::context ctx;
};

VarSolver::VarSolver() : _impl(new Impl) {}

VarSolver::~VarSolver() { delete _impl; }

std::map<string, string> VarSolver::resolve(const VarBlock& block,
                                            ContextManager* context_manager,
                                            std::mt19937_64& gen) {
    try {
        Resolver resolver(_impl->ctx, context_manager, gen);
        return resolver.resolve(block);
    } catch (const z3::exception& e) {
        throw NoiseValueError(format("VARS solver: {}", e.msg()));
    }
}

};  // namespace noise
