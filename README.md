# Noise

Noise is a **non-deterministic macro processor**. You give it:

1. **Macro definitions** (`.def` files) - each macro expands by its own rules,
   which may involve randomness;
2. **A document template** in which those macros are called;
3. **A seed**.

The output is the template with every macro expanded. Each seed produces a
different document - different numbers, choices or wording - but always
within the structure of the template. The same seed always produces the same
document.

```
$ cat examples/greet.def
# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# A greeting with a default or a randomly picked salutation (see README.md)
MACRO greeting = {
    TEXT = "salutation, name!";
    PARAMS = { salutation = Hello; };
    ARGS = { required name = ""; };
};
MACRO salutation_pick = { TEXT = "select(text=Hello Hi Howdy)"; };

$ cat examples/letter.in
greeting(World)
greeting<salutation=salutation_pick>(Alice)

$ noise -d examples/greet.def -s 1 examples/letter.in
Hello, World!
Howdy, Alice!
$ noise -d examples/greet.def -s 2 examples/letter.in
Hello, World!
Hello, Alice!
```

## Building

Requirements: a C++23 compiler, CMake, Bison, Flex and the Z3 solver's
development package (`libz3-dev` on Ubuntu). The dev container
(`.devcontainer/`) has all of them.

```
cmake -S . -B build/main
cmake --build build/main
ctest --test-dir build/main          # regression tests
```

The executable is `build/main/src/noise`. Example files used in this README
are in `examples/`; the commands below run from the repository root.

## Command line

```
noise [options] [template files...]
```

Template files are expanded in the order given; with none, the template is
read from stdin. Options take effect in command line order: an option applies
to the template files that follow it.

| Option | Meaning |
|---|---|
| `-h`, `--help` | print help |
| `-v`, `--version` | print the version |
| `-d`, `--def <file>` | load a macro definitions file; may be repeated, files load in order |
| `-I`, `--include-path <dir>[:<dir>...]` | search path for `INCLUDE`d def files |
| `-D`, `--define <name>[=<value>]` | define a global param (see [Global params](#global-params)) |
| `-s`, `--seed <n>` | random seed (default 0) |
| `-o`, `--out-file <file>` | write the output of the template files that follow there (default stdout); a later `-o` switches to another file |
| `-l`, `--log-file <file>` | log file (default stderr) |
| `--log-level <level>` | `fatal`, `error`, `warning`, `info`, `debug`, or a number 10-50 |

On an error noise prints a single `Noise-F-...` line to stderr and exits with
status 1.

## Document templates

A template is plain text in which macro calls are expanded; everything else is
copied to the output as is.

### Calling a macro

A macro call is the macro's name, optionally followed by a param list in angle
brackets and/or an arg list in parentheses:

```
name
name<params>
name(args)
name<params>(args)
```

**Params** are expanded (their own macro calls resolved) *before* the macro
runs; **args** are passed to the macro as written. Both lists are
comma-separated and may be empty. Each entry has the form

```
[attrs] [name=] value
```

- An entry with `name=` binds the macro's param/arg of that name; entries
  without a name bind the remaining ones in order.
- A value runs up to the next unescaped `,` or closing bracket. Nested
  `(...)` and `<...>` groups are kept whole, so `f(a,b)` is one value.
- Leading and trailing whitespace of a value is trimmed.

**Attrs** modify a single entry (they can also be given on a macro's own
definition of the param/arg):

| Attr | Effect |
|---|---|
| `required` | the entry must be given (in a definition) - required entries come first |
| `quoted` | the value is quoted with `"` or `'`; the quotes are dropped |
| `keep_left_ws` (`klws`) | don't trim leading whitespace |
| `keep_right_ws` (`krws`) | don't trim trailing whitespace |
| `keep_enclosing_ws` (`kws`) | don't trim either side |
| `bool` | the value is a boolean (see below) |

A `bool` value accepts, case-insensitively, `true`, `t`, `yes`, `y`, `1`, `ok`
or `false`, `f`, `no`, `n`, `0`. Anything else warns and falls back to the
param's default. For a `bool` param, a bare name alone means true: `dups` is
the same as `dups=true`.

### Escapes and quotes

`\` makes the next character literal, everywhere in a template: in text, in
values and in quotes. It is how you write a macro's name as plain text, or a
`,` inside a value (`\,`). A literal backslash is `\\`. Here `dice` is a
macro from [`examples/dice.def`](examples/dice.def) (shown in
[Variables and constraints](#variables-and-constraints)):

```
$ printf '%s\n' '\dice is plain text, dice is not; a literal \\ needs two' | noise -d examples/dice.def -s 1
dice is plain text, 6 + 5 = 11 is not; a literal \ needs two
```

Text in `'...'` or `"..."` is never expanded. At document level the quotes are
kept in the output; inside a param/arg value they just delimit it.

### How expansion works

When a macro is called, its params are expanded first, then all params and
args (with defaults for any not given) become names in scope, and the
macro's text is expanded with them. The result is re-expanded until it no
longer changes - so a macro's output may itself contain macro calls - and
then replaces the call.

## Macro definition files

A `.def` file defines macros, and may include other def files. Keywords are
case-insensitive; `#` starts a comment to the end of the line; `\` escapes the
next character.

```
INCLUDE <common.def>;

MACRO name = {
    TEXT = "...";                    # required: the macro's template text
    PARAMS = { [attrs] name = value; ... };
    ARGS   = { [attrs] name = value; ... };
    VARS   = { ... };                # see Variables and constraints
};

VARS = { ... };                      # global variables
```

The `value` of a param/arg is its default. It needs quoting only to contain
`;`, `#`, a quote, exact leading/trailing whitespace, or several lines.
Quoted text takes three equivalent forms:

```
text = <<EOT
  foo()
  bar
EOT
;
text = "
  foo()
  bar
";
text = '
  foo()
  bar
';
```

`INCLUDE <file>` looks for the file next to the including file, then along
`-I`. Circular includes are an error.

### Inside a macro's text

A macro's params and args are available by name and by position
(`param[0]`, `arg[1]`, ...). Four read-only values are always available:

| Name | Value |
|---|---|
| `#params`, `#args` | how many params/args the call passed |
| `#line`, `#col` | where in the output this macro's expansion starts |

Any name in scope is replaced wherever it appears as a word in the text, so
give params and variables names that won't collide with ordinary words.

## Builtin macros

### Random numbers

Each distribution is a macro with its parameters as params. All of them also
take `fmt`: a [`std::format`](https://en.cppreference.com/w/cpp/utility/format/spec)
format spec - the part after `:` in `{:...}` - e.g. `fmt=.0f` for no decimals
or, for the integer distributions, `fmt=x` for hex.

| Macro | Params | Samples |
|---|---|---|
| `uniform_dist` | `a` (0), `b` (1); `a <= b` | a real number in `[a, b]` |
| `exp_dist` | `lambda` (1); `> 0` | exponential, rate `lambda` |
| `normal_dist` | `mean` (0), `stddev` (1); `stddev > 0` | Gaussian |
| `binomial_dist` | `t` (1) `>= 0`, `p` (0.5) in `[0,1]` | successes in `t` trials |
| `poisson_dist` | `mean` (1); `> 0` | a Poisson event count |

An out-of-range parameter is an error.

### Text

| Macro | Params | Args | Does |
|---|---|---|---|
| `repeat` | | `text` (required), `count` (1) | `text` repeated `count` times |
| `select` | `unit` (`word`), `count` (1), `dups` (false), `sorted` (false) | `text` (required) | picks `count` random items of `text` |
| `foreach` | `unit` (`word`), `var` (`item`) | `items`, `text` (both required) | expands `text` once per item, with `var` set to it |

A `unit` is `word`, `line` or `paragraph`. `select` picks each item at most
once, in random order; `dups` allows repeats (implied when `count` exceeds the
number of items) and `sorted` keeps the source order.

```
$ echo 'uniform_dist<a=1, b=6, fmt=.0f> | select<count=2>(text=red green blue) | repeat(kws text="ab ", count=3)' | noise -s 3
4 | green blue | ab ab ab
$ echo 'foreach<unit=line>(items="apples
pears", kws text="- item
")' | noise
- apples
- pears
```

## Variables and constraints

Variables are typed values chosen at random - per seed - such that
constraints you state hold. They are declared in a `VARS` clause: at the top
level of a def file (global, resolved once before the document is expanded)
or inside a macro (resolved on every call, after its params, so constraints
may use them).

```
$ cat examples/dice.def
# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
# Two dice whose sum is at least 10 (see README.md, Variables and constraints)
MACRO dice = {
    TEXT = "a + b = sum";
    VARS = {
        UINT a, b, sum;
        ASSERT a >= 1 AND a <= 6 AND b >= 1 AND b <= 6;
        ASSERT sum == a + b AND sum >= 10;
    };
};
$ for s in 1 2 3; do echo dice | noise -d examples/dice.def -s $s; done
6 + 5 = 11
4 + 6 = 10
6 + 4 = 10
```

Once resolved, variables work like params: their names are replaced by their
values in the text, and they are visible to macros called from there. An inner
scope's variable of the same name hides an outer one.

### Declarations

```
TYPE name [= fallback] [, name [= fallback]]... ;
```

| Type | Values | Operators (besides `==`, `!=`) |
|---|---|---|
| `BOOL` | `true` / `false` | `NOT` `AND` `OR` `XOR` `->` (implies) |
| `INT` | 64-bit signed | `+ - * / %` `& \| ^ ~ << >>` `< <= > >=` |
| `UINT` | 64-bit unsigned | as `INT` |
| `REAL` | a real number | `+ - * /` `< <= > >=` |
| `BITVEC[w]` | a `w`-bit unsigned | as `INT` |
| `STRING` | one of the block's string literals | |

A **STRING** variable can only take one of the string literals appearing in
its `VARS` block (in constraints or as fallbacks).

A **fallback** value is used if the constraints can't be satisfied (below).

### Constraints

`ASSERT expression;` adds a constraint. Precedence is C-like, highest first:

1. bit selects `b[i]` `b[hi:lo]`
2. unary `-` `~` `NOT`, casts `(TYPE)x`
3. `*` `/` `%`
4. `+` `-`
5. `<<` `>>`
6. `<` `<=` `>` `>=`
7. `==` `!=`
8. `&`, then `^`, then `|`
9. `AND`, then `XOR`, then `OR`
10. `->`

As in C, `/` truncates toward zero and `>>` on an `INT` keeps the sign.

**Bit selects** pick bits of a `BITVEC`, Verilog style: `b[0]` is the least
significant bit, `b[i]` is bit `i` (a `BITVEC[1]`), and `b[hi:lo]` is bits `hi`
down to `lo`, both included (a `BITVEC[hi-lo+1]`). They work on any `BITVEC`
expression - `(a ^ b)[3:0]` - and on other integers through a cast:
`((BITVEC[64])i)[7:0]`. An index is an integer literal or a param's name; it
can't be a variable. A reversed range (`b[3:7]`), an index beyond the width,
or a bit select of a non-`BITVEC` is an error when the variables are resolved.

```
VARS = {
    BITVEC[8] v;
    ASSERT v[7:6] == 2 AND v[0] == 1 AND v[5:1] == 0;   # v is 129
};
```

Bit selects exist only in constraints. In a macro's text, `b[0]` is the
variable's value followed by the text `[0]`; to print some of its bits, give
them a variable of their own (`BITVEC[4] high; ASSERT high == b[7:4];`).

A name is a variable only if it was declared *earlier in the same block*. Any
other name is looked up among the params, global params and outer variables
in scope (as a constant), and otherwise read as a string literal. Such values
and plain literals take the type of what they are combined with. Two
variables of different types need an explicit cast, e.g. `(UINT)i`.

`SMTLIB "...";` adds raw [SMT-LIB](https://smt-lib.org/) assertions, which may
refer to the variables declared before it. `INT` and `UINT` are 64-bit
bitvectors there, so they need bitvector operations (`bvult`, `bvadd`, ...).

### Randomness and failure

Each variable gets a random value consistent with the constraints and with
the values already chosen. When a variable's valid values form one range,
every value in it is equally likely. A `REAL` variable is picked within
`+-NOISE_REAL_RANGE` when its constraints allow that.

If the constraints can't be satisfied, every variable of the block takes its
fallback value; if any of them has none, noise stops with an error.

## Global params

`-D name=value` defines a param visible everywhere - in the document, in every
macro and in all `VARS` constraints. `-D name` alone means `name=true`. A
later definition of a name overrides an earlier one. Values are taken
literally, not expanded.

```
$ echo 'Dear who,' | noise -D who=Bob
Dear Bob,
```

## Configuration params

Noise's own limits and tunables are params whose names start with `NOISE_`.
Like any param, they can be set globally with `-D`, as a macro's param
default, or at a call (`m<NOISE_MAX_PROBES=0>`), and apply to everything
expanded within that scope.

| Param | Default | Meaning |
|---|---|---|
| `NOISE_MAX_DEPTH` | 256 | max macro nesting depth - a macro calling itself fails here instead of crashing |
| `NOISE_MAX_EXPANSIONS` | 64 | max times a macro's output is re-expanded before it must stop changing |
| `NOISE_MAX_PROBES` | 128 | solver checks spent picking one variable's random value; `0` takes the solver's first solution, the same for every seed |
| `NOISE_REAL_RANGE` | 1e9 | a `REAL` variable is picked within `[-range, range]` |

```
$ echo 'dice / dice' | noise -d examples/dice.def -D NOISE_MAX_PROBES=0 -s 1
5 + 6 = 11 / 5 + 6 = 11
```

Each probe is a solver check, and checks involving `*`, `/` or `%` on 64-bit
`INT`/`UINT` variables are relatively expensive; a smaller `BITVEC[w]` is
cheaper when the range allows it.
