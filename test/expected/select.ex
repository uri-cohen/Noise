1. default is unsorted (selection order): beta alpha
2. sorted=true restores original text order: beta gamma
3. a bare sorted flag means sorted=true: beta delta
4. a count above the item count implies dups: b b c c b b
5. explicit dups=true allows repeats: b b e
6. a bare dups flag also allows repeats: c d d
7. dups=false (default) never repeats when count fits: a d e
8. an invalid bool value warns and falls back to the default: b c
9. a plain bare word is still a positional value, not a flag: [hello]
