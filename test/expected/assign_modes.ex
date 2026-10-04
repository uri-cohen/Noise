Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. inside a macro with n=7: plain = [n=1] [n=2]; caller := [n=7]; callee := [n=7] [n=7]; =! [n=2]; defined(n) true
2. at top level, n undefined: [n=2] [n=1] [n=2] defined(n) false
3. args too, inside c=9: [c=9] [c=9] [c=5]; at top level: [c=0] [c=5]
4. inherited from a -D param: [d=42] [d=42] [d=2]
5. a value starting with !: [n=!x] [n=!y]
6. defined skips its own scope: false false true
