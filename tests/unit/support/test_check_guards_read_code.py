#!/usr/bin/env python3
"""The code-read rule has to be able to fail, and on this repository it cannot.

Every registered guard in the tree passes, which is exactly the state in which a meta-check
is indistinguishable from a no-op. These cases run the same findings() function against
synthetic guards, so each rule has a red it produces.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from check_guards_read_code import findings  # noqa: E402

CLEAN = '''#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_hash_code, read_raw

BACKEND = Path("components") / "xy_backend.c"
CMAKE = Path("components") / "CMakeLists.txt"


def main() -> int:
    source = read_code(BACKEND)
    cmake = read_hash_code(CMAKE)
    return 0 if "HAL_Init(" in source and "xy_backend.c" in cmake else 1
'''


class GuardsReadCodeTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "tests" / "unit" / "hal").mkdir(parents=True)

    def write(self, name: str, text: str) -> str:
        path = self.root / "tests" / "unit" / "hal" / name
        path.write_text(text, encoding="utf-8")
        return path.relative_to(self.root).as_posix()

    def test_a_clean_guard_passes(self) -> None:
        entry = self.write("check_clean.py", CLEAN)
        self.assertEqual(findings(self.root, (entry,), {}), [])

    def test_a_c_variable_read_through_read_text_is_reported(self) -> None:
        entry = self.write("check_raw.py", CLEAN.replace(
            "    source = read_code(BACKEND)\n", "    source = BACKEND.read_text()\n"))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("BACKEND" in p and "read_text" in p for p in problems), problems)

    def test_a_c_literal_read_through_read_text_is_reported(self) -> None:
        entry = self.write("check_literal.py", CLEAN.replace(
            "    source = read_code(BACKEND)\n",
            '    source = Path("components/xy_backend.c").read_text()\n'))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("literal" in p for p in problems), problems)

    def test_a_registered_guard_without_the_import_is_reported(self) -> None:
        entry = self.write("check_noimport.py", CLEAN.replace(
            'from source_scan import read_code, read_hash_code, read_raw\n', ""))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("does not import source_scan" in p for p in problems), problems)

    def test_a_registered_guard_that_never_strips_is_reported(self) -> None:
        entry = self.write("check_nostrip.py", CLEAN.replace("read_code(BACKEND)", "read_raw(BACKEND)"))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("never calls read_code" in p for p in problems), problems)

    def test_a_hash_variable_read_through_read_raw_is_reported(self) -> None:
        entry = self.write("check_cmake_raw.py", CLEAN.replace(
            "read_hash_code(CMAKE)", "read_raw(CMAKE)"))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("CMAKE" in p and "read_raw" in p for p in problems), problems)

    def test_a_hash_literal_inside_read_raw_is_reported(self) -> None:
        entry = self.write("check_kconfig_raw.py", CLEAN.replace(
            "    cmake = read_hash_code(CMAKE)\n", '    cmake = read_raw(Path("Kconfig"))\n'))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("Kconfig" in p for p in problems), problems)

    def test_a_hash_read_through_read_text_is_reported(self) -> None:
        entry = self.write("check_cmake_literal.py", CLEAN.replace(
            "    cmake = read_hash_code(CMAKE)\n",
            '    cmake = Path("components/CMakeLists.txt").read_text()\n'))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("CMakeLists.txt" in p and "read_text" in p for p in problems),
                        problems)

    def test_a_hash_read_with_a_stated_reason_is_allowed(self) -> None:
        entry = self.write("check_prose.py", CLEAN.replace(
            "read_hash_code(CMAKE)",
            "read_raw(CMAKE)  # prose: the rule requires a rationale comment to be present"))
        self.assertEqual([], findings(self.root, (entry,), {}))

    def test_a_hash_read_with_an_empty_reason_is_reported(self) -> None:
        entry = self.write("check_prose_empty.py", CLEAN.replace(
            "read_hash_code(CMAKE)", "read_raw(CMAKE)  # prose:"))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("stating a reason" in p for p in problems), problems)

    def test_a_hash_file_that_is_prose_throughout_can_be_exempt(self) -> None:
        entry = self.write("check_generated.py", CLEAN.replace(
            "read_hash_code(CMAKE)", "read_raw(CMAKE)"))
        reason = {entry: "asserts on the generated cmake output, which is the product"}
        self.assertEqual([], findings(self.root, (entry,), {}, reason))

    def test_a_hash_file_exemption_without_a_reason_is_reported(self) -> None:
        entry = self.write("check_generated.py", CLEAN.replace(
            "read_hash_code(CMAKE)", "read_raw(CMAKE)"))
        problems = findings(self.root, (entry,), {}, {entry: "   "})
        self.assertTrue(any("stated reason" in p for p in problems), problems)

    def test_a_walk_that_classifies_cmake_files_is_still_a_hash_read(self) -> None:
        # `read_raw(path)` inside `if name == "CMakeLists.txt"` names nothing on its own line.
        entry = self.write("check_walk.py", CLEAN.replace(
            "    cmake = read_hash_code(CMAKE)\n",
            "    cmake = \"\"\n"
            "    for name in (\"CMakeLists.txt\",):\n"
            "        if name == \"CMakeLists.txt\":\n"
            "            cmake = read_raw(CMAKE)\n"))
        problems = findings(self.root, (entry,), {})
        self.assertTrue(any("read_raw" in p for p in problems), problems)

    def test_a_repeated_registration_is_reported(self) -> None:
        entry = self.write("check_twice.py", CLEAN)
        problems = findings(self.root, (entry, entry), {})
        self.assertTrue(any("listed 2 times" in p for p in problems), problems)

    def test_a_stale_registration_is_reported(self) -> None:
        problems = findings(self.root, ("tests/unit/hal/check_gone.py",), {})
        self.assertTrue(any("file is gone" in p for p in problems), problems)

    def test_an_unregistered_c_reader_is_reported(self) -> None:
        # The realistic shape: a new guard that reads raw text and was never registered.
        entry = self.write("check_unknown.py", CLEAN.replace(
            "    source = read_code(BACKEND)\n", "    source = BACKEND.read_text()\n"))
        problems = findings(self.root, (), {})
        self.assertTrue(any(entry in p and "neither" in p for p in problems), problems)

    def test_an_exemption_with_a_reason_passes(self) -> None:
        entry = self.write("check_manifest.py", 'p = Path("a.c")\nprint(p.read_text())\n')
        self.assertEqual(findings(self.root, (), {entry: "reads a manifest"}), [])

    def test_an_exemption_without_a_reason_is_reported(self) -> None:
        entry = self.write("check_manifest.py", 'p = Path("a.c")\nprint(p.read_text())\n')
        problems = findings(self.root, (), {entry: "   "})
        self.assertTrue(any("no stated reason" in p for p in problems), problems)

    def test_a_stale_exemption_is_reported(self) -> None:
        problems = findings(self.root, (), {"tests/unit/hal/check_gone.py": "was a manifest"})
        self.assertTrue(any("exempted" in p and "file is gone" in p for p in problems), problems)

    def test_a_file_cannot_be_registered_and_exempt(self) -> None:
        entry = self.write("check_both.py", CLEAN)
        problems = findings(self.root, (entry,), {entry: "claimed exempt"})
        self.assertTrue(any("both" in p for p in problems), problems)


if __name__ == "__main__":
    unittest.main()
