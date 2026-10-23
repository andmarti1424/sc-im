"""Exercise history persistence through the real curses interface (POSIX)."""

import errno
import fcntl
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import tempfile
import termios
import time
import unittest


BINARY = Path(__file__).resolve().parents[1] / "src" / "sc-im"


class HistoryFileTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = self.root / "config" / "sc-im"
        self.config.mkdir(parents=True)
        self.cache = self.root / "cache"
        self.cache.mkdir()
        self.history = self.cache / "sc-iminfo"

    def run_session(self, keys=b":q!\n", config="", args=(), xdg=True):
        config_dir = self.config if xdg else self.root / ".config" / "sc-im"
        config_dir.mkdir(parents=True, exist_ok=True)
        (config_dir / "scimrc").write_text(config)
        env = dict(os.environ, HOME=str(self.root), TERM="xterm", LC_ALL="C.UTF-8")
        env.pop("XDG_CONFIG_HOME", None)
        env.pop("XDG_CACHE_HOME", None)
        if xdg:
            env.update(XDG_CONFIG_HOME=str(self.config.parent), XDG_CACHE_HOME=str(self.cache))
        master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
        output = bytearray()
        process = subprocess.Popen(
            [str(BINARY), *args], stdin=slave, stdout=slave, stderr=slave,
            env=env, cwd=self.root, start_new_session=True,
            preexec_fn=lambda: fcntl.ioctl(0, termios.TIOCSCTTY, 0),
        )
        os.close(slave)
        try:
            deadline = time.monotonic() + 10
            sent = False
            while process.poll() is None:
                if time.monotonic() > deadline:
                    self.fail("sc-im did not exit: " + repr(bytes(output[-1500:])))
                if select.select([master], [], [], 0.1)[0]:
                    try:
                        chunk = os.read(master, 65536)
                    except OSError as error:
                        if error.errno == errno.EIO:
                            break
                        raise
                    output.extend(chunk)
                    self.assertNotIn(b"syntax error", bytes(output).lower())
                    # Wait for the initial sheet, rather than sending input
                    # while curses is still initializing the terminal.
                    if not sent and b"Sheet1" in output:
                        os.write(master, keys)
                        sent = True
            self.assertEqual(process.wait(timeout=2), 0, repr(bytes(output[-1500:])))
            self.assertNotIn(b"syntax error", bytes(output).lower())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)

    def test_default_saves_both_histories(self):
        self.run_session(b":int let A0=42\n=17\n:q!\n")
        text = self.history.read_text()
        self.assertIn(":int let A0=42\n", text)
        self.assertIn("=17\n", text)

    def test_config_disables_file_creation(self):
        self.cache.rmdir()
        self.run_session(b":int let A0=42\n=17\n:q!\n", "set history_file=0\n")
        self.assertFalse(self.history.exists())
        self.assertFalse(self.cache.exists())

    def test_disabled_leaves_existing_file_untouched(self):
        original = b":int let A0=73\n=9\n"
        self.history.write_bytes(original)
        modified = self.history.stat().st_mtime_ns
        self.run_session(b":int let A0=42\n=17\n:q!\n", "set nohistory_file\n")
        self.assertEqual(self.history.read_bytes(), original)
        self.assertEqual(self.history.stat().st_mtime_ns, modified)

    def test_disabled_does_not_load_existing_commands(self):
        self.history.write_text(":int let A0=73\n")
        self.run_session(b":\x10\n:w result.sc\n:q!\n", "set history_file=0\n")
        self.assertNotIn("let A0", (self.root / "result.sc").read_text())

    def test_default_loads_existing_commands(self):
        self.history.write_text(":int let A0=73\n")
        self.run_session(b":\x10\n:w result.sc\n:q!\n")
        self.assertIn("let A0 = 73", (self.root / "result.sc").read_text())

    def test_default_loads_existing_insert_history(self):
        self.history.write_text("=73\n")
        self.run_session(b"=\x10\n:w result.sc\n:q!\n")
        self.assertIn("let A0 = 73", (self.root / "result.sc").read_text())

    def test_disabled_does_not_load_existing_insert_history(self):
        self.history.write_text("=73\n")
        self.run_session(b"=\x10\n:w result.sc\n:q!\n", "set nohistory_file\n")
        self.assertNotIn("let A0", (self.root / "result.sc").read_text())

    def test_session_recall_still_works(self):
        self.run_session(
            b":int let A0=42\n:int let A0=0\n:\x10\x10\n:w result.sc\n:q!\n",
            "set nohistory_file\n",
        )
        self.assertIn("let A0 = 42", (self.root / "result.sc").read_text())
        self.assertFalse(self.history.exists())

    def test_runtime_disable(self):
        self.run_session(b":set nohistory_file\n:int let A0=42\n:q!\n")
        self.assertFalse(self.history.exists())

    def test_runtime_reenable(self):
        self.run_session(b":set history_file\n:int let A0=42\n:q!\n", "set nohistory_file\n")
        self.assertIn(":int let A0=42\n", self.history.read_text())

    def test_home_fallback(self):
        self.run_session(config="set nohistory_file\n", xdg=False)
        self.assertFalse((self.root / ".cache" / "sc-iminfo").exists())

    def test_command_line_disable(self):
        self.run_session(args=("--history_file=0",))
        self.assertFalse(self.history.exists())

    def test_config_execute_does_not_duplicate_saved_history(self):
        self.history.write_text(":int let A0=73\n")
        self.run_session(b":w result.sc\n:q!\n", config='execute "int let A0=5"\n')
        self.assertIn("let A0 = 5", (self.root / "result.sc").read_text())
        self.assertEqual(self.history.read_text().count(":int let A0=73\n"), 1)

    def test_config_execute_respects_disabled_history(self):
        original = b":int let A0=73\n=9\n"
        self.history.write_bytes(original)
        self.run_session(config='set nohistory_file\nexecute "int let A0=5"\n')
        self.assertEqual(self.history.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
