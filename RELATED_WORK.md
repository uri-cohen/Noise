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
| **[Dada Engine](http://dev.null.org/dadaengine/)** (Andrew Bulhak; the "Postmodernism Generator"[^pomo]) | Grammar-based random text, with rules, parameters and variables | Very close in purpose; grammar rules instead of a document template with macro calls |
| **[Rant](https://github.com/rant-lang/rant)** | A language for procedural text: blocks, random choice, variables, seeds | Close; richer text features (e.g. capitalization rules), no constraint solving |
| **[Tracery](https://tracery.io/)**[^tracery] (Kate Compton) | JSON grammars with `#symbol#` expansion and modifiers, popular for text bots | Simpler; no params, numeric distributions or constraints |
| **[RiTa / RiScript](https://rednoise.org/rita/)**[^rita] | A text generation library with inline choice and variable syntax | Library-first, for natural language |
| **[Polygen](https://polygen.org/)**, **[rmutt](https://sourceforge.net/projects/rmutt/)**, **[SCIgen](https://pdos.csail.mit.edu/archive/scigen/)** | Context-free grammar sentence and document generators | Pure random grammars; no macros or constraints |
| **[Perchance](https://perchance.org/)** | Web-based random generators built from lists and templates | Interactive and hobbyist-oriented |

## Constrained random generation

Where noise's `VARS` come from.

| Project | What it is | Compared with noise |
|---|---|---|
| **[SystemVerilog](https://standards.ieee.org/ieee/1800/7743/) constrained random** (`rand`, `constraint {}`, `randomize()`, UVM[^uvm]) | The hardware verification standard: typed random variables, constraint blocks, solver-backed and seeded | The conceptual twin of `VARS`, down to bit vectors and Verilog-style `b[hi:lo]` bit selects - but it produces stimulus objects, not text |
| **[CRAVE](https://github.com/agra-uni-bremen/crave)** (C++, for SystemC) | Constrained random using SMT/BDD solvers | The same idea as a C++ library |
| **[PyVSC](https://github.com/fvutils/pyvsc)**, **[constrainedrandom](https://github.com/imaginationtech/constrainedrandom)** (Python) | SystemVerilog-style constraints in Python, solver-backed | The same idea as Python libraries |
| **[ISLa](https://github.com/rindPHI/isla)** (from *The Fuzzing Book*[^isla]) | Grammars plus SMT constraints (Z3) to generate structured inputs | Probably the closest combination: a structure description plus solved constraints - aimed at fuzzing program inputs rather than document templates |

## Template engines and macro processors

The same expansion model, but deterministic.

| Project | Compared with noise |
|---|---|
| **[m4](https://www.gnu.org/software/m4/)**, **[GPP](https://logological.org/gpp)** (generic preprocessor), **[cpp](https://gcc.gnu.org/onlinedocs/cpp/)** | The same family: macros with args expanded in text, m4 even re-expanding results. No randomness or constraints |
| **[Jinja2](https://jinja.palletsprojects.com/)**, **[Mako](https://www.makotemplates.org/)**, **[Mustache](https://mustache.github.io/)** | Template engines; randomness only through user-supplied functions |
| **[envsubst](https://www.gnu.org/software/gettext/manual/html_node/envsubst-Invocation.html)** | Substitutes `$NAME` from the environment - as noise's `$name` lookup does for environment variables |

## Fake and test data generation

| Project | Compared with noise |
|---|---|
| **[Faker](https://faker.readthedocs.io/)**[^faker] (Python and others), **[Mimesis](https://mimesis.name/)** | Seeded realistic data (names, addresses), with format templates; data-first, not document-first |
| **[Hypothesis](https://hypothesis.works/)**, **[QuickCheck](https://hackage.haskell.org/package/QuickCheck)** generators | Seeded random structured values for property-based testing |
| **[Grammarinator](https://github.com/renatahodovan/grammarinator)**, **[Domato](https://github.com/googleprojectzero/domato)**, **[Dharma](https://github.com/MozillaSecurity/dharma)** | Grammar-based fuzzers generating random documents |

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

[^pomo]: The Postmodernism Generator, the Dada Engine's best known use:
    <https://www.elsewhere.org/journal/pomo/>
[^tracery]: Source code: <https://github.com/galaxykate/tracery>
[^rita]: Source code (JavaScript): <https://github.com/dhowe/ritajs>
[^uvm]: The Universal Verification Methodology (Accellera), the standard
    SystemVerilog verification library: <https://www.accellera.org/downloads/standards/uvm>
[^isla]: The Fuzzing Book chapter on ISLa, "Fuzzing with Constraints":
    <https://www.fuzzingbook.org/html/FuzzingWithConstraints.html>
[^faker]: Source code (Python): <https://github.com/joke2k/faker>
