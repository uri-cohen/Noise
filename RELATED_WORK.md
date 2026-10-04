# Related work

Noise combines three things that are usually found apart:

1. **Text macro expansion** - `$name` macros with params and args, expanded in a
   document template, their output re-expanded until stable (as in m4);
2. **Seeded randomness** - every seed yields a different document of the same
   structure (as in random text generators);
3. **Constrained random variables** - typed variables whose values a solver
   (Z3) picks at random so that stated constraints hold (as in hardware
   verification).

No single tool we know of combines all three. Each has close relatives,
listed below by family. (Compiled from general knowledge as of 2026, not from
an exhaustive survey - newer projects may be missing.)

## Random text generation

The closest in spirit: templates or grammars from which each run, or seed,
yields a different text of the same structure.

| Project | What it is | Compared with noise |
|---|---|---|
| **Dada Engine** (Andrew Bulhak; the "Postmodernism Generator") | Grammar-based random text, with rules, parameters and variables | Very close in purpose; grammar rules instead of a document template with macro calls |
| **Rant** | A language for procedural text: blocks, random choice, variables, seeds | Close; richer text features (e.g. capitalization rules), no constraint solving |
| **Tracery** (Kate Compton) | JSON grammars with `#symbol#` expansion and modifiers, popular for text bots | Simpler; no params, numeric distributions or constraints |
| **RiTa / RiScript** | A text generation library with inline choice and variable syntax | Library-first, for natural language |
| **Polygen**, **rmutt**, **SCIgen** | Context-free grammar sentence and document generators | Pure random grammars; no macros or constraints |
| **Perchance** | Web-based random generators built from lists and templates | Interactive and hobbyist-oriented |

## Constrained random generation

Where noise's `VARS` come from.

| Project | What it is | Compared with noise |
|---|---|---|
| **SystemVerilog constrained random** (`rand`, `constraint {}`, `randomize()`, UVM) | The hardware verification standard: typed random variables, constraint blocks, solver-backed and seeded | The conceptual twin of `VARS`, down to bit vectors and Verilog-style `b[hi:lo]` bit selects - but it produces stimulus objects, not text |
| **CRAVE** (C++, for SystemC) | Constrained random using SMT/BDD solvers | The same idea as a C++ library |
| **PyVSC**, **constrainedrandom** (Python) | SystemVerilog-style constraints in Python, solver-backed | The same idea as Python libraries |
| **ISLa** (from *The Fuzzing Book*) | Grammars plus SMT constraints (Z3) to generate structured inputs | Probably the closest combination: a structure description plus solved constraints - aimed at fuzzing program inputs rather than document templates |

## Template engines and macro processors

The same expansion model, but deterministic.

| Project | Compared with noise |
|---|---|
| **m4**, **GPP** (generic preprocessor), **cpp** | The same family: macros with args expanded in text, m4 even re-expanding results. No randomness or constraints |
| **Jinja2**, **Mako**, **Mustache** | Template engines; randomness only through user-supplied functions |
| **envsubst** | Substitutes `$NAME` from the environment - as noise's `$name` lookup does for environment variables |

## Fake and test data generation

| Project | Compared with noise |
|---|---|
| **Faker** (Python and others), **Mimesis** | Seeded realistic data (names, addresses), with format templates; data-first, not document-first |
| **Hypothesis**, **QuickCheck** generators | Seeded random structured values for property-based testing |
| **Grammarinator**, **Domato**, **Dharma** | Grammar-based fuzzers generating random documents |

## Where noise stands

- **Against random text generators** (Dada Engine, Rant, Tracery): noise starts
  from a document template with macros rather than a grammar, and adds
  solver-backed constraints between values.
- **Against constrained random tools** (SystemVerilog, CRAVE, PyVSC): noise
  produces text, and its constraints serve document generation.
- **Against ISLa**, the nearest neighbor in mechanism: ISLa fuzzes program inputs
  from a grammar, while noise expands human-written templates.

For positioning, the natural comparisons are **Dada Engine / Rant** (random
text), **SystemVerilog constrained random** (constraints) and **ISLa** (the
combination).
