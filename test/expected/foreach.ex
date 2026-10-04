Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. default var name "item", word unit, default trimming collapses the templates own spacing: [alpha][beta][gamma]
2. custom var name, kws text preserves the templates own trailing space: (x) (y) (z) 
3. line unit: - one
- two
- three

4. paragraph unit, nested macro call: <[first para
still first]>
<[second para]>

5. items as a param are expanded before splitting: <1><2><3>
