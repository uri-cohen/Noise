Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. double backslash is a literal backslash: a\b
2. a single backslash escapes the next char and is consumed: axb
3. escaping the first letter of a macro name prints it literally: show
4. an unescaped macro call still works normally: [Z]
5. an escaped comma stays literal inside a value: [a,b]
6. escaping required leaves it as literal value text, not an attr: [required]
7. an escaped left angle bracket cannot start a param list: []<oops>
8. an escaped hash is just a literal hash: a#b
