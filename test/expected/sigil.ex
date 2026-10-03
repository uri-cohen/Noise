Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. plain words: greet who num
2. names: hi world (plain who and greet stay words; first param: world) / hi you (plain who and greet stay words; first param: you) / 42
3. environment: hello from the environment
4. literal $: $greet, $5, US$ 3, a lone $ sign
5. literal $ through macros: echo $HOME costs $5 / $HOME$HOME
6. unknown names expand to nothing: [] [] []
