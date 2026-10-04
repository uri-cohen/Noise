// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// Variables and constraints: the parsed (solver independent) form of a
/// VARS clause. See VarSolver.h for its resolution.

#pragma once

#include <noise_std_include.h>
#include <variant>

namespace noise {

enum class VarKind { BOOL, INT, UINT, REAL, BITVEC, STRING };

struct VarType {
    VarKind kind = VarKind::STRING;
    // BITVEC only - INT/UINT are 64 bit wide implicitly
    unsigned width = 0;

    bool operator==(const VarType& o) const = default;

    bool is_bv() const {
        return kind == VarKind::INT || kind == VarKind::UINT ||
               kind == VarKind::BITVEC;
    }
    bool is_numeric() const { return is_bv() || kind == VarKind::REAL; }
    unsigned bv_width() const { return kind == VarKind::BITVEC ? width : 64; }
    std::string str() const;
};

/// @brief A node of a constraint expression tree (ASSERT's operand).
/// LITERAL/NAME leaves are "untyped" - they adopt the type of the operand
/// they are combined with. A NAME is an identifier that was not declared
/// (before) in its VARS block: it is looked up in the ContextManager at
/// solve time and, if not found there, used as a string literal.
struct Expr {
    enum class Op {
        LITERAL, VAR, NAME,
        NEG, BITNOT, NOT, CAST,
        MUL, DIV, MOD, ADD, SUB, SHL, SHR,
        LT, LE, GT, GE, EQ, NE,
        BITAND, BITXOR, BITOR, AND, XOR, OR, IMPLIES,
        // Verilog-like bit select of a BITVEC: INDEX b[i] (kids: b, i) and
        // SLICE b[hi:lo] (kids: b, hi, lo). Indices are LITERAL or NAME
        // leaves holding non-negative integers (a VAR is rejected at solve
        // time - Z3's extract needs constants).
        INDEX, SLICE,
    };

    Op op;
    std::string text;  // LITERAL text or VAR/NAME identifier
    VarType type;      // CAST target type
    std::vector<std::shared_ptr<const Expr>> kids;

    static std::shared_ptr<const Expr> leaf(Op op, const std::string& text);
    static std::shared_ptr<const Expr> unary(Op op,
                                             std::shared_ptr<const Expr> a);
    static std::shared_ptr<const Expr> binary(Op op,
                                              std::shared_ptr<const Expr> a,
                                              std::shared_ptr<const Expr> b);
    static std::shared_ptr<const Expr> cast(const VarType& type,
                                            std::shared_ptr<const Expr> a);
    static std::shared_ptr<const Expr> slice(std::shared_ptr<const Expr> base,
                                             std::shared_ptr<const Expr> hi,
                                             std::shared_ptr<const Expr> lo);
};

using ExprPtr = std::shared_ptr<const Expr>;

const char* op_name(Expr::Op op);

struct VarDecl {
    std::string name;
    VarType type;
    std::optional<std::string> fallback;  // canonical form (see canonical_value)
};

struct VarAssert {
    ExprPtr expr;
};

struct VarSmtlib {
    std::string code;
};

using VarItem = std::variant<VarDecl, VarAssert, VarSmtlib>;

/// @brief One VARS clause - its declarations, constraints and SMTLIB code,
/// kept in source order ("declared before" matters for both an ASSERT and
/// the declarations an SMTLIB chunk can see).
class VarBlock {
  public:
    /// Throws NoiseDefBuilderError on a duplicate name or on a fallback
    /// value not valid for the type.
    void add_decl(const std::string& name, const VarType& type,
                  const std::optional<std::string>& fallback);
    void add_assert(ExprPtr expr);
    void add_smtlib(const std::string& code);

    bool declared(const std::string& name) const {
        return _names.contains(name);
    }
    const std::vector<VarItem>& items() const { return _items; }

  private:
    std::vector<VarItem> _items;
    std::set<std::string> _names;
};

/// Case insensitive boolean spellings - for BOOL variables and "bool" params
/// alike: true, t, yes, y, 1, ok, on / false, f, no, n, 0, off.
std::optional<bool> parse_bool(const std::string& text);

/// @brief Parses text as a value of the given type.
/// @return its canonical string form (the one a resolved variable of that
/// type is rendered as), or nullopt if text is not a valid value.
std::optional<std::string> canonical_value(const VarType& type,
                                           const std::string& text);

/// Integer/unsigned parsing (decimal or 0x hex) used by canonical_value and
/// by the solver when lifting an untyped literal.
std::optional<int64_t> parse_int64(const std::string& text);
std::optional<uint64_t> parse_uint64(const std::string& text);
std::optional<double> parse_real(const std::string& text);

};  // namespace noise
