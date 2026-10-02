// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <charconv>
#include <cmath>
//
#include <Exception.h>
#include <Utils.h>
#include <Vars.h>

using std::format;
using std::string;

namespace noise {

string VarType::str() const {
    switch (kind) {
    case VarKind::BOOL: return "BOOL";
    case VarKind::INT: return "INT";
    case VarKind::UINT: return "UINT";
    case VarKind::REAL: return "REAL";
    case VarKind::BITVEC: return format("BITVEC[{}]", width);
    case VarKind::STRING: return "STRING";
    }
    return "?";
}

ExprPtr Expr::leaf(Op op, const string& text) {
    auto e = std::make_shared<Expr>();
    e->op = op;
    e->text = text;
    return e;
}

ExprPtr Expr::unary(Op op, ExprPtr a) {
    auto e = std::make_shared<Expr>();
    e->op = op;
    e->kids = {a};
    return e;
}

ExprPtr Expr::binary(Op op, ExprPtr a, ExprPtr b) {
    auto e = std::make_shared<Expr>();
    e->op = op;
    e->kids = {a, b};
    return e;
}

ExprPtr Expr::cast(const VarType& type, ExprPtr a) {
    auto e = std::make_shared<Expr>();
    e->op = Op::CAST;
    e->type = type;
    e->kids = {a};
    return e;
}

ExprPtr Expr::slice(ExprPtr base, ExprPtr hi, ExprPtr lo) {
    auto e = std::make_shared<Expr>();
    e->op = Op::SLICE;
    e->kids = {base, hi, lo};
    return e;
}

const char* op_name(Expr::Op op) {
    using enum Expr::Op;
    switch (op) {
    case LITERAL: return "literal";
    case VAR: return "variable";
    case NAME: return "name";
    case NEG: return "-";
    case BITNOT: return "~";
    case NOT: return "NOT";
    case CAST: return "cast";
    case MUL: return "*";
    case DIV: return "/";
    case MOD: return "%";
    case ADD: return "+";
    case SUB: return "-";
    case SHL: return "<<";
    case SHR: return ">>";
    case LT: return "<";
    case LE: return "<=";
    case GT: return ">";
    case GE: return ">=";
    case EQ: return "==";
    case NE: return "!=";
    case BITAND: return "&";
    case BITXOR: return "^";
    case BITOR: return "|";
    case AND: return "AND";
    case XOR: return "XOR";
    case OR: return "OR";
    case IMPLIES: return "->";
    case INDEX: return "[i]";
    case SLICE: return "[hi:lo]";
    }
    return "?";
}

void VarBlock::add_decl(const string& name, const VarType& type,
                        const std::optional<string>& fallback) {
    if (_names.contains(name)) {
        throw NoiseDefBuilderError(
            format("variable '{}' declared twice in the same VARS", name));
    }
    VarDecl decl{name, type, std::nullopt};
    if (fallback) {
        decl.fallback = canonical_value(type, *fallback);
        if (!decl.fallback) {
            throw NoiseDefBuilderError(
                format("invalid {} fallback value '{}' for variable '{}'",
                       type.str(), *fallback, name));
        }
    }
    _names.insert(name);
    _items.emplace_back(std::move(decl));
}

void VarBlock::add_assert(ExprPtr expr) { _items.emplace_back(VarAssert{expr}); }

void VarBlock::add_smtlib(const string& code) {
    _items.emplace_back(VarSmtlib{code});
}

std::optional<bool> parse_bool(const string& text) {
    static const std::set<string> TRUE_STRINGS = {"true", "t", "yes", "y", "1", "ok"};
    static const std::set<string> FALSE_STRINGS = {"false", "f", "no", "n", "0"};
    string v = trim(text);
    for (auto& c : v) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    if (TRUE_STRINGS.contains(v)) {
        return true;
    }
    if (FALSE_STRINGS.contains(v)) {
        return false;
    }
    return std::nullopt;
}

std::optional<uint64_t> parse_uint64(const string& text) {
    string v = trim(text);
    int base = 10;
    if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        v = v.substr(2);
        base = 16;
    }
    if (v.empty()) {
        return std::nullopt;
    }
    uint64_t out = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out, base);
    if (ec != std::errc() || ptr != v.data() + v.size()) {
        return std::nullopt;
    }
    return out;
}

std::optional<int64_t> parse_int64(const string& text) {
    string v = trim(text);
    bool neg = false;
    if (!v.empty() && (v[0] == '-' || v[0] == '+')) {
        neg = v[0] == '-';
        v = v.substr(1);
    }
    auto mag = parse_uint64(v);
    if (!mag) {
        return std::nullopt;
    }
    constexpr uint64_t MAX_POS = static_cast<uint64_t>(INT64_MAX);
    if (neg) {
        if (*mag > MAX_POS + 1) {
            return std::nullopt;
        }
        return static_cast<int64_t>(0 - *mag);
    }
    if (*mag > MAX_POS) {
        return std::nullopt;
    }
    return static_cast<int64_t>(*mag);
}

std::optional<double> parse_real(const string& text) {
    string v = trim(text);
    if (v.empty()) {
        return std::nullopt;
    }
    double out = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (ec != std::errc() || ptr != v.data() + v.size() || !std::isfinite(out)) {
        return std::nullopt;
    }
    return out;
}

std::optional<string> canonical_value(const VarType& type, const string& text) {
    switch (type.kind) {
    case VarKind::BOOL:
        if (auto b = parse_bool(text)) {
            return *b ? "true" : "false";
        }
        return std::nullopt;
    case VarKind::INT:
        if (auto i = parse_int64(text)) {
            return std::to_string(*i);
        }
        return std::nullopt;
    case VarKind::UINT:
        if (auto u = parse_uint64(text)) {
            return std::to_string(*u);
        }
        return std::nullopt;
    case VarKind::BITVEC:
        if (type.width <= 64) {
            auto u = parse_uint64(text);
            if (!u || (type.width < 64 && (*u >> type.width) != 0)) {
                return std::nullopt;
            }
            return std::to_string(*u);
        } else {
            // wider than 64 bits: decimal digits only, taken modulo 2^width
            string v = trim(text);
            if (v.empty() || !std::ranges::all_of(v, [](char c) {
                    return std::isdigit(static_cast<unsigned char>(c));
                })) {
                return std::nullopt;
            }
            auto nz = v.find_first_not_of('0');
            return nz == string::npos ? "0" : v.substr(nz);
        }
    case VarKind::REAL:
        if (auto d = parse_real(text)) {
            return format("{}", *d);
        }
        return std::nullopt;
    case VarKind::STRING:
        return text;
    }
    return std::nullopt;
}

};  // namespace noise
