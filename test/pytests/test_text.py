
import sys
import os
import pytest

sys.path.insert(0, ".")
from noise_api import *

class TestText:

    data_dir = '../../../test/data'
    test_data = [('basic','basic'),('args','args')]

    @pytest.mark.parametrize('def_name,text_name', test_data)
    @pytest.mark.pb
    def test_basic(self, def_name, text_name):
        d = TestText.data_dir
        noise = Noise()
        noise.process_def_file(f'{d}/{def_name}.def')
        with open(f'{d}/{text_name}.in') as in_f:
            out = noise.expand_string(in_f.read())
        with open(f'{d}/{text_name}.expected') as expected_f:
            expected = expected_f.read()
        assert out == expected
