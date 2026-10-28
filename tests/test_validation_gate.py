"""Exercise the actual shell gate with fake validator processes."""
import pathlib
import subprocess
import tempfile
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / 'scripts/validate-plugin.sh'

class ValidatorGateTest(unittest.TestCase):
    def check(self, output, code, expected):
        with tempfile.TemporaryDirectory() as folder:
            folder = pathlib.Path(folder)
            validator = folder / 'fake pluginval'
            validator.write_text('#!/usr/bin/env bash\nprintf "%s\\n" "' + output + '"\nexit ' + str(code) + '\n')
            validator.chmod(0o755)
            log = folder / 'logs/plugin.log'
            result = subprocess.run(['bash', str(SCRIPT), str(validator), 'plugin with spaces.vst3', str(log)], capture_output=True)
            self.assertEqual(result.returncode == 0, expected)
            self.assertTrue(log.is_file())

    def test_success(self): self.check('SUCCESS', 0, True)
    def test_success_then_crash(self): self.check('SUCCESS', 139, False)
    def test_no_verdict(self): self.check('all tests started', 0, False)
    def test_reported_failure(self): self.check('SUCCESS\n!!! Test failed', 0, False)
    def test_failed_process(self): self.check('FAILURE', 1, False)

    def test_crash_then_success_is_retried(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = pathlib.Path(folder)
            marker = folder / 'crashed'
            validator = folder / 'flaky pluginval'
            validator.write_text('#!/usr/bin/env bash\nif [ ! -e "' + str(marker) + '" ]; then touch "' + str(marker) + '"; echo SUCCESS; exit 139; fi\necho SUCCESS\n')
            validator.chmod(0o755)
            result = subprocess.run(['bash', str(SCRIPT), str(validator), 'p.vst3', str(folder / 'p.log')], capture_output=True)
            self.assertEqual(result.returncode, 0)

if __name__ == '__main__':
    unittest.main()
