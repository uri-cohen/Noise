// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <charconv>
#include <cmath>
//
#include <Exception.h>
#include <NumExpr.h>

using std::format;
using std::string;

namespace noise {

namespace {

using Type = NumValue::Type;

NumValue make_int(int64_t i) { NumValue v; v.type = Type::INT; v.i = i; return v; }
NumValue make_real(double d) { NumValue v; v.type = Type::REAL; v.d = d; return v; }
NumValue make_bool(bool b) { NumValue v; v.type = Type::BOOL; v.b = b; return v; }

// Recursive descent over the constraint precedence, lowest first.
class Parser {
  public:
    explicit Parser(const string& text) : _s(text) {}

    NumValue parse() {
        NumValue v = implies();
        skip_ws();
        if (_p < _s.size()) {
            fail(format("unexpected '{}'", _s.substr(_p, 1)));
        }
        return v;
    }

  private:
    const string& _s;
    size_t _p = 0;

    [[noreturn]] void fail(const string& what) const {
        throw NoiseValueError(
            format("expr: {} at position {} in '{}'", what, _p + 1, _s));
    }

    void skip_ws() {
        while (_p < _s.size() && std::isspace(static_cast<unsigned char>(_s[_p]))) {
            ++_p;
        }
    }

    // Matches the operator symbol `op` - but not as the start of a longer one
    // (e.g. "<" in "<<" or "<=", "-" in "->").
    bool op(const char* op) {
        skip_ws();
        size_t n = std::strlen(op);
        if (_s.compare(_p, n, op) != 0) {
            return false;
        }
        char next = _p + n < _s.size() ? _s[_p + n] : '\0';
        string sym(op);
        if ((sym == "<" && (next == '<' || next == '=')) ||
            (sym == ">" && (next == '>' || next == '=')) ||
            (sym == "-" && next == '>')) {
            return false;
        }
        _p += n;
        return true;
    }

    // Matches the (case insensitive) keyword `kw` as a whole word.
    bool kw(const char* kw) {
        skip_ws();
        size_t n = std::strlen(kw);
        if (_p + n > _s.size()) {
            return false;
        }
        for (size_t i = 0; i < n; ++i) {
            if (std::tolower(static_cast<unsigned char>(_s[_p + i])) != kw[i]) {
                return false;
            }
        }
        if (_p + n < _s.size() &&
            (std::isalnum(static_cast<unsigned char>(_s[_p + n])) || _s[_p + n] == '_')) {
            return false;
        }
        _p += n;
        return true;
    }

    bool need_bool(const NumValue& v, const char* what) const {
        if (v.type != Type::BOOL) {
            fail(format("'{}' needs true/false operands, not a number", what));
        }
        return v.b;
    }

    void need_num(const NumValue& v, const char* what) const {
        if (v.type == Type::BOOL) {
            fail(format("'{}' needs numbers, not true/false", what));
        }
    }

    void need_int(const NumValue& v, const char* what) const {
        if (v.type != Type::INT) {
            fail(format("'{}' needs integers", what));
        }
    }

    static long double as_real(const NumValue& v) {
        return v.type == Type::INT ? static_cast<long double>(v.i) : v.d;
    }

    NumValue real_result(long double d) const {
        if (!std::isfinite(static_cast<double>(d))) {
            fail("the result is not a finite number");
        }
        return make_real(static_cast<double>(d));
    }

    NumValue implies() {
        NumValue l = or_();
        if (op("->")) {
            NumValue r = implies();
            return make_bool(!need_bool(l, "->") || need_bool(r, "->"));
        }
        return l;
    }

    NumValue or_() {
        NumValue l = xor_();
        while (kw("or")) {
            NumValue r = xor_();
            l = make_bool(need_bool(l, "OR") | need_bool(r, "OR"));
        }
        return l;
    }

    NumValue xor_() {
        NumValue l = and_();
        while (kw("xor")) {
            NumValue r = and_();
            l = make_bool(need_bool(l, "XOR") != need_bool(r, "XOR"));
        }
        return l;
    }

    NumValue and_() {
        NumValue l = bitor_();
        while (kw("and")) {
            NumValue r = bitor_();
            l = make_bool(need_bool(l, "AND") & need_bool(r, "AND"));
        }
        return l;
    }

    NumValue bitor_() {
        NumValue l = bitxor_();
        while (op("|")) {
            NumValue r = bitxor_();
            need_int(l, "|"), need_int(r, "|");
            l = make_int(l.i | r.i);
        }
        return l;
    }

    NumValue bitxor_() {
        NumValue l = bitand_();
        while (op("^")) {
            NumValue r = bitand_();
            need_int(l, "^"), need_int(r, "^");
            l = make_int(l.i ^ r.i);
        }
        return l;
    }

    NumValue bitand_() {
        NumValue l = equality();
        while (op("&")) {
            NumValue r = equality();
            need_int(l, "&"), need_int(r, "&");
            l = make_int(l.i & r.i);
        }
        return l;
    }

    NumValue equality() {
        NumValue l = relational();
        for (;;) {
            bool eq;
            if (op("==")) eq = true;
            else if (op("!=")) eq = false;
            else return l;
            NumValue r = relational();
            bool same;
            if (l.type == Type::BOOL || r.type == Type::BOOL) {
                if (l.type != r.type) {
                    fail("can't compare true/false with a number");
                }
                same = l.b == r.b;
            } else {
                same = as_real(l) == as_real(r);
                if (l.type == Type::INT && r.type == Type::INT) same = l.i == r.i;
            }
            l = make_bool(eq ? same : !same);
        }
    }

    NumValue relational() {
        NumValue l = shift();
        for (;;) {
            const char* o;
            if (op("<=")) o = "<=";
            else if (op(">=")) o = ">=";
            else if (op("<")) o = "<";
            else if (op(">")) o = ">";
            else return l;
            NumValue r = shift();
            need_num(l, o), need_num(r, o);
            int c;
            if (l.type == Type::INT && r.type == Type::INT) {
                c = (l.i > r.i) - (l.i < r.i);
            } else {
                long double a = as_real(l), b = as_real(r);
                c = (a > b) - (a < b);
            }
            string s(o);
            l = make_bool(s == "<" ? c < 0 : s == "<=" ? c <= 0 : s == ">" ? c > 0 : c >= 0);
        }
    }

    NumValue shift() {
        NumValue l = additive();
        for (;;) {
            bool left;
            if (op("<<")) left = true;
            else if (op(">>")) left = false;
            else return l;
            NumValue r = additive();
            const char* o = left ? "<<" : ">>";
            need_int(l, o), need_int(r, o);
            if (r.i < 0 || r.i > 63) {
                fail(format("shift amount {} is out of range (0..63)", r.i));
            }
            // like the VARS INT bitvectors: << wraps, >> keeps the sign
            l = make_int(left ? static_cast<int64_t>(static_cast<uint64_t>(l.i) << r.i)
                              : l.i >> r.i);
        }
    }

    NumValue additive() {
        NumValue l = multiplicative();
        for (;;) {
            bool add;
            if (op("+")) add = true;
            else if (op("-")) add = false;
            else return l;
            NumValue r = multiplicative();
            const char* o = add ? "+" : "-";
            need_num(l, o), need_num(r, o);
            if (l.type == Type::INT && r.type == Type::INT) {
                int64_t out;
                if (add ? __builtin_add_overflow(l.i, r.i, &out)
                        : __builtin_sub_overflow(l.i, r.i, &out)) {
                    fail(format("integer overflow in {} {} {}", l.i, o, r.i));
                }
                l = make_int(out);
            } else {
                l = real_result(add ? as_real(l) + as_real(r) : as_real(l) - as_real(r));
            }
        }
    }

    NumValue multiplicative() {
        NumValue l = unary();
        for (;;) {
            char o;
            if (op("*")) o = '*';
            else if (op("/")) o = '/';
            else if (op("%")) o = '%';
            else return l;
            NumValue r = unary();
            const char os[2] = {o, '\0'};
            need_num(l, os), need_num(r, os);
            if (o == '%') {
                need_int(l, "%"), need_int(r, "%");
            }
            if (l.type == Type::INT && r.type == Type::INT) {
                if (o != '*' && r.i == 0) {
                    fail("division by zero");
                }
                int64_t out;
                if (o == '*') {
                    if (__builtin_mul_overflow(l.i, r.i, &out)) {
                        fail(format("integer overflow in {} * {}", l.i, r.i));
                    }
                } else if (l.i == INT64_MIN && r.i == -1) {
                    if (o == '/') {
                        fail(format("integer overflow in {} / -1", l.i));
                    }
                    out = 0;
                } else {
                    out = o == '/' ? l.i / r.i : l.i % r.i;
                }
                l = make_int(out);
            } else {
                long double b = as_real(r);
                if (o == '/' && b == 0) {
                    fail("division by zero");
                }
                l = real_result(o == '*' ? as_real(l) * b : as_real(l) / b);
            }
        }
    }

    NumValue unary() {
        if (op("-")) {
            NumValue v = unary();
            need_num(v, "-");
            if (v.type == Type::REAL) return make_real(-v.d);
            if (v.i == INT64_MIN) fail("integer overflow in negation");
            return make_int(-v.i);
        }
        if (op("~")) {
            NumValue v = unary();
            need_int(v, "~");
            return make_int(~v.i);
        }
        if (kw("not")) {
            NumValue v = unary();
            return make_bool(!need_bool(v, "NOT"));
        }
        return primary();
    }

    NumValue primary() {
        skip_ws();
        if (op("(")) {
            NumValue v = implies();
            if (!op(")")) {
                fail("missing ')'");
            }
            return v;
        }
        if (kw("true")) return make_bool(true);
        if (kw("false")) return make_bool(false);
        if (_p >= _s.size()) {
            fail("missing an operand");
        }
        char c = _s[_p];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            return number();
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t e = _p;
            while (e < _s.size() &&
                   (std::isalnum(static_cast<unsigned char>(_s[e])) || _s[e] == '_')) {
                ++e;
            }
            fail(format("'{}' is not a number (a name not expanded? use ${})",
                        _s.substr(_p, e - _p), _s.substr(_p, e - _p)));
        }
        fail(format("unexpected '{}'", string(1, c)));
    }

    NumValue number() {
        const char* begin = _s.data() + _p;
        const char* end = _s.data() + _s.size();
        if (_s.compare(_p, 2, "0x") == 0 || _s.compare(_p, 2, "0X") == 0) {
            uint64_t u = 0;
            auto [ptr, ec] = std::from_chars(begin + 2, end, u, 16);
            if (ptr == begin + 2) fail("a hex number without digits");
            if (ec != std::errc() || u > static_cast<uint64_t>(INT64_MAX)) {
                fail("an integer out of the int64 range");
            }
            _p += ptr - begin;
            return after_number(make_int(static_cast<int64_t>(u)));
        }
        // a real if it has a '.' or an exponent, else an integer
        size_t e = _p;
        while (e < _s.size() && std::isdigit(static_cast<unsigned char>(_s[e]))) ++e;
        bool real = e < _s.size() && (_s[e] == '.' || _s[e] == 'e' || _s[e] == 'E');
        if (real) {
            double d = 0;
            auto [ptr, ec] = std::from_chars(begin, end, d);
            if (ec != std::errc() || !std::isfinite(d)) fail("a malformed real number");
            _p += ptr - begin;
            return after_number(make_real(d));
        }
        int64_t i = 0;
        auto [ptr, ec] = std::from_chars(begin, end, i);
        if (ec != std::errc()) fail("an integer out of the int64 range");
        _p += ptr - begin;
        return after_number(make_int(i));
    }

    // A number must not run into a name ("3x").
    NumValue after_number(NumValue v) const {
        if (_p < _s.size() &&
            (std::isalnum(static_cast<unsigned char>(_s[_p])) || _s[_p] == '_' ||
             _s[_p] == '.')) {
            fail(format("unexpected '{}' after a number", string(1, _s[_p])));
        }
        return v;
    }
};

}  // namespace

NumValue evaluate_num_expr(const string& text) {
    return Parser(text).parse();
}

};  // namespace noise
