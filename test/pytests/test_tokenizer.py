# Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

import sys
import os
import pytest

sys.path.insert(0, ".")

from noise_api import *

class TestTokenizer:
    t_list = [("<", Token_LT),
              (">", Token_GT),
              ("  ", Token_TEXT),
              ("(", Token_LP),
              (")", Token_RP),
              ("aa", Token_ID),
              ("    ", Token_TEXT),
              ("{", Token_LB),
              ("}", Token_RB),
              ("[", Token_LSB),
              ("]", Token_RSB),
              (" ", Token_TEXT),
              ("=", Token_EQ),
              ("'", Token_Q),
              ("\"", Token_QQ),
              (",", Token_COMMA),
              ("bb", Token_ID)
              ]
    t_str_list, t_token_list = [p[0] for p in t_list], [p[1] for p in t_list]
    t_str = ''.join(t_str_list)

    @pytest.mark.pb
    def test_dora(self):
        stkr = StringTokenizer(TestTokenizer.t_str, "<t_list>")
        tkr = stkr.tokenizer()
        i = 0
        while (tc := tkr.next()) is not None:
            assert tc.token == TestTokenizer.t_token_list[i]
            assert tc.str == TestTokenizer.t_str_list[i]
            i += 1
        assert i == len(TestTokenizer.t_list)


    def test_eat_ws(self):
        src_txt = "A <> \n\t ()"
        stkr = StringTokenizer(src_txt, "<eat_ws>")
        tkr = stkr.tokenizer()
        tokens_w_ws = []
        str_w_ws = []
        while (tc := tkr.next()) is not None:
            tokens_w_ws.append(tc.token)
            str_w_ws.append(tc.str)
        assert tokens_w_ws == [Token_ID, Token_TEXT, Token_LT, Token_GT,
                               Token_TEXT, Token_NL, Token_TEXT,
                               Token_LP, Token_RP, Token_EOS]
        assert ''.join(str_w_ws) == src_txt

        stkr = StringTokenizer(src_txt, "<eat_ws>")
        tkr = stkr.tokenizer()
        tokens_no_ws = []
        str_no_ws = []
        expected = MakeTokenSet([
            Token_ID, Token_LT, Token_GT, Token_LP, Token_RP, Token_EOS])
        while (tc := tkr.next()) is not None:
            tkr.eat_ws(expected)
            tc = tkr.current()
            tokens_no_ws.append(tc.token)
            str_no_ws.append(tc.str)
        assert tokens_no_ws == [Token_ID, Token_LT, Token_GT,
                               Token_LP, Token_RP, Token_EOS]
        assert ''.join(str_no_ws) == "A<>()"


    def test_capture_q_text(self):
        src_txt = "ab 'foo <7> (bar,4) ' == 2"
        stkr = StringTokenizer(src_txt, "<quoted_text>")
        tkr = stkr.tokenizer()
        q_txt, o_txt = [], []
        while (tc := tkr.next()) is not None:
            if tc.token == Token_Q:
                q_txt = tkr.capture_quoted_text(tc)
            else:
                o_txt.append(tc.str)
        assert q_txt == "'foo <7> (bar,4) '"
        assert ''.join(o_txt) == "ab  == 2"


    def test_capture_until(self):
        src_txt = "const std::set<Token,std::less<Token>,std::allocator<Token>>& right_tokens"
        stkr = StringTokenizer(src_txt, "<quoted_text>")
        tkr = stkr.tokenizer()
        gt_set = MakeTokenSet([Token_GT])
        while (tc := tkr.next()) is not None:
            if tc.token == Token_LT:
                in_text = tkr.capture_until(gt_set)
                assert in_text == "Token,std::less<Token>,std::allocator<Token>"
