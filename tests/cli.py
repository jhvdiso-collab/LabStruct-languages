"""Contract tests: each query starts a fresh process and reloads the file."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

EXE = str(pathlib.Path(sys.argv.pop(1)).resolve())
GO = '--go' in sys.argv
if GO:
    sys.argv.remove('--go')


class CLI(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.file = pathlib.Path(self.tmp.name) / 'database with spaces.data'

    def query(self, command, expected=None, fail=False):
        before = self.file.read_bytes() if self.file.exists() else None
        p = subprocess.run([EXE, '--file', str(self.file), '--query', command],
                           capture_output=True, text=True, timeout=10)
        if fail:
            self.assertNotEqual(p.returncode, 0, command)
            self.assertIn('Error:', p.stderr)
            self.assertEqual(before, self.file.read_bytes() if self.file.exists() else None)
        else:
            self.assertEqual(p.returncode, 0, f'{command}: {p.stderr}')
            self.assertEqual(p.stdout.strip(), expected, command)
        return p

    def test_empty_creation(self):
        for p in ('MFLSQ' if GO else 'MFLSQT'):
            name = p.lower()
            self.query(f'{p}CREATE {name}', 'OK')
            self.query(f'PRINT {name}', '[]')
            self.query(f'{p}CREATE {name}', fail=True)
        if not GO:
            self.query('TGET t 0', 'FALSE')
            self.query('TDEL t 0', 'FALSE')

    def test_array(self):
        self.query('MCREATE a', 'OK')
        self.query('PRINT a', '[]')
        self.query('MPUSH a 20', '20')
        self.query('MINSERT a 0 10', '10')
        self.query('MINSERT a 2 30', '30')
        self.query('MINSERT a 1 15', '15')
        self.query('PRINT a', '[10 15 20 30]')
        self.query('MGET a 2', '20')
        self.query('MSET a 2 -1', '-1')
        self.query('MDEL a 1', '15')
        self.query('MLEN a', '3')
        self.query('PRINT a', '[10 -1 30]')
        for command in ['MGET a -1', 'MDEL a 3', 'MSET a 3 1', 'MINSERT a 4 1']:
            self.query(command, fail=True)

    def test_lists(self):
        for p in 'FL':
            name = p.lower()
            self.query(f'{p}CREATE {name}', 'OK')
            self.query(f'{p}PUSH {name} 20', '20')
            self.query(f'{p}PUSHFRONT {name} 10', '10')
            self.query(f'{p}PUSHBACK {name} 40', '40')
            self.query(f'{p}PUSHBEFORE {name} 2 30', '30')
            self.query(f'{p}PUSHAFTER {name} 1 25', '25')
            self.query(f'PRINT {name}', '[10 20 25 30 40]')
            self.query(f'{p}REVERSE {name}', '[40 30 25 20 10]')
            self.query(f'{p}GET {name} 2', '25')
            self.query(f'{p}FIND {name} 30', '3')
            self.query(f'{p}FIND {name} 99', '-1')
            self.query(f'{p}LEN {name}', '5')
            for command in [f'{p}DELBEFORE {name} 0', f'{p}DELAFTER {name} 4',
                            f'{p}PUSHAFTER {name} -1 3', f'{p}PUSHBEFORE {name} 5 3']:
                self.query(command, fail=True)
            self.query(f'{p}DELBEFORE {name} 2', '20')
            self.query(f'{p}DELAFTER {name} 1', '30')
            self.query(f'{p}DELVAL {name} 25', 'TRUE')
            self.query(f'{p}DELVAL {name} 99', 'FALSE')
            self.query(f'{p}DELFRONT {name}', '10')
            self.query(f'{p}DELBACK {name}', '40')
            self.query(f'PRINT {name}', '[]')
            self.query(f'{p}DELBACK {name}', fail=True)
            self.query(f'{p}PUSH {name} 7', '7')
            self.query(f'{p}PUSH {name} 7', '7')
            self.query(f'{p}DELVAL {name} 7', 'TRUE')
            self.query(f'PRINT {name}', '[7]')
            self.query(f'{p}DEL {name} 0', '7')

    def test_stack_queue(self):
        for p, order in [('S', [3, 2, -1]), ('Q', [-1, 2, 3])]:
            name = p.lower()
            for v in [-1, 2, 3]:
                self.query(f'{p}PUSH {name} {v}', str(v))
            self.query(f'PRINT {name}', '[' + ' '.join(map(str, order)) + ']')
            self.query(f'{p}GET {name}', str(order[0]))
            for v in order:
                self.query(f'{p}POP {name}', str(v))
            self.query(f'{p}POP {name}', fail=True)
            self.query(f'{p}GET {name}', fail=True)
            self.query(f'{p}PUSH {name} 42', '42')
            self.query(f'{p}POP {name}', '42')

    @unittest.skipIf(GO, 'AVL belongs to C++ per specification')
    def test_avl(self):
        for v in [10, 20, 30, 40, 50, 25]:
            self.query(f'TINSERT t {v}', 'TRUE')
        self.query('PRINT t', '[10 20 25 30 40 50]')
        self.query('TINSERT t 25', 'FALSE')
        self.query('TGET t 25', 'TRUE')
        self.query('ISMEMBER t 99', 'FALSE')
        for v in [30, 10, 50, 20, 25, 40]:
            self.query(f'TDEL t {v}', 'TRUE')
            self.query(f'TGET t {v}', 'FALSE')
        self.query('TDEL t 99', 'FALSE')
        self.query('PRINT t', '[]')

    def test_invalid_commands(self):
        self.query('PRINT missing', fail=True)
        self.query('MPUSH a 2147483648', fail=True)
        self.query('MPUSH a nope', fail=True)
        self.query('MPUSH a 1 extra', fail=True)
        self.query('MPUSH bad/name 1', fail=True)
        self.query('MPUSH a -2147483648', '-2147483648')
        self.query('MPUSH a 2147483647', '2147483647')
        for command in ['QPUSH a 1', 'MCREATE a', 'BOGUS a', '', 'MPUSH a', 'MGET a 1x']:
            self.query(command, fail=True)
        self.query('PRINT a', '[-2147483648 2147483647]')

    def test_corrupt_file(self):
        for data in ['', 'wrong\n', 'LABSTRUCT 1\nM a 2 1\n',
                     'LABSTRUCT 1\nM a 1 1 extra\n', 'LABSTRUCT 1\nX a 0\n',
                     'LABSTRUCT 1\nM a 0\nM a 0\n', 'LABSTRUCT 1\nT t 2 1 1\n',
                     'LABSTRUCT 1\nM a -1\n', 'LABSTRUCT 1\nM a 1 2147483648\n']:
            self.file.write_text(data)
            self.query('MPUSH b 1', fail=True)

    def test_read_does_not_rewrite(self):
        self.query('MPUSH a 1', '1')
        before = self.file.stat().st_mtime_ns
        self.query('MGET a 0', '1')
        self.assertEqual(before, self.file.stat().st_mtime_ns)
        self.assertEqual(list(self.file.parent.glob('*.tmp.*')), [])

    def test_options_and_save_error(self):
        for args in [[], ['--file'], ['--bad', 'x'], ['--file', 'a', '--file', 'b', '--query', 'PRINT a']]:
            p = subprocess.run([EXE, *args], capture_output=True, text=True, timeout=10)
            self.assertNotEqual(p.returncode, 0)
        self.file = pathlib.Path(self.tmp.name) / 'missing-directory' / 'data'
        self.query('MPUSH a 1', fail=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
