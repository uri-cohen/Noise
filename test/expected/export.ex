Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. to the caller: [ x=5] / top level:  x=5
2. global, from deep:  g=deep
3. to a named calling macro: a(b(c) b:x=from_c) a:x=from_c / not calling, skipped: y=[]
4. no value:  n=3 flag=true
5. a computed value:  total=14
6. removed in between: outer:x=1  outer:x=clobbered then x=clobbered
7. builtins do not count as callers: [count=3]
8. a config param:  0 1 2
9. := inherits an export: [k=default]  [k=exported]
10. a call in the output sees the export: [z=1]
