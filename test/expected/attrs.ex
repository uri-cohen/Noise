Copyrights Uri Cohen uri.l.cohen@gmail.com 2026
1. default (trim is automatic): got=[hi]
2. quoted (trim no-op, no surrounding ws): got=[hi]
3. keep_left_ws keeps the left side only: got=[   hi]
4. keep_right_ws (krws) keeps the right side only: got=[hi  ]
5. keep_enclosing_ws keeps both sides: got=[   hi  ]
6. keep_left_ws + keep_right_ws (klws+krws) == keep_enclosing_ws: got=[   hi  ]
7. call-site keep_left_ws on a positional arg: raw=[   hi]
8. call-site kws (alias) on a positional arg: raw=[   hi  ]
9. named param: got=world count=1
10. dup text: ababab
11. pick words: beta alpha
12. fixed random: 5
