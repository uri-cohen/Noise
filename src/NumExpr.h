// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// Evaluation of a numeric expression given as text (the expr macro): the
/// VARS constraint operators and precedence, over int64 integers, reals and
/// booleans.

#pragma once

#include <noise_std_include.h>

namespace noise {

struct NumValue {
    enum class Type { INT, REAL, BOOL };
    Type type = Type::INT;
    int64_t i = 0;
    double d = 0;
    bool b = false;
};

/// @brief Evaluates `text` - numbers (decimal or 0x hex integers, reals),
/// true/false, parentheses and, highest precedence first: unary - ~ NOT;
/// * / %; + -; << >>; < <= > >=; == !=; &; ^; |; AND; XOR; OR; -> (right
/// associative). Integer arithmetic is int64 with C semantics (/ truncates,
/// % takes the dividend's sign, >> keeps the sign); a real operand makes it
/// real. Throws NoiseValueError on a malformed expression, a leftover name, a
/// type mismatch, division by zero or integer overflow.
NumValue evaluate_num_expr(const std::string& text);

};  // namespace noise
