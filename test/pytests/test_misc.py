
import sys
import os
import pytest

sys.path.insert(0, ".")
import noise_api

class TestMisc:

    @pytest.mark.pb
    def test_load_noise(self):
        dir_noise_set = set(dir(noise_api))
        for c in ['Noise', 'Macro', 'MacroInst']:
            assert c in dir_noise_set
