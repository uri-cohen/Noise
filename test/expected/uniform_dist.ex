Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. default a=0,b=1,fmt=(empty) gives a raw real number: 0.15979336337046085
2. positional a,b with a 2-decimal fmt: 19.92
3. named a=b collapses to a fixed value: 7
4. a negative-to-positive range formatted as an integer: 1
5. fmt can pad/align like any std::format spec:  1.542
