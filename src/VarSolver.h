// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

/// Resolution of a VARS block with the Z3 solver. Z3 itself is used only by
/// VarSolver.cpp - no Z3 header leaks out of it.

#pragma once

#include <noise_std_include.h>

namespace noise {

class VarBlock;
class ContextManager;

/// @brief Solves the block's constraints and picks a random (per `gen`)
/// consistent value for each of its variables.
///
/// Identifiers not declared in the block are looked up in context_manager
/// (params, outer scope variables) and otherwise used as string literals.
/// If the constraints are unsatisfiable (or undecided) every variable takes
/// its fallback value; a variable without one raises NoiseValueError.
/// @return var-name -> value (canonical string form, see canonical_value)
std::map<std::string, std::string> resolve_vars(const VarBlock& block,
                                                ContextManager* context_manager,
                                                std::mt19937_64& gen);

};  // namespace noise
