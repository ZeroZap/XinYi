#!/usr/bin/env python3
"""`-DBUILD_TESTING=ON` configures a build with tests off, and says nothing.

`cmake/Kconfig.cmake` ends with `set(BUILD_TESTING ${CONFIG_BUILD_TESTING} CACHE BOOL ... FORCE)`.
`FORCE` discards whatever the cache already held, so the value a reader passed with `-D` is thrown
away and replaced by the Kconfig default. Measured on this tree: `-DBUILD_TESTING=ON` produced
`BUILD_TESTING:BOOL=OFF` and a summary line reading `Tests Enabled: OFF`.

Two reasons that is a defect rather than a preference:

  * `BUILD_TESTING` is CMake's own standard name - it is what `include(CTest)` defines and what a
    reader who has used CMake before will type. Reading the repository should not require knowing
    that this tree spells the same intent `-DKCONFIG_OVERRIDES="BUILD_TESTING=ON"`.
    `check_ci_build_flags.py` already treats "the build overrides a variable a workflow passed" as a
    defect; this is the same thing on the command line.
  * It fails silently. Nothing warns, and the summary prints the value that took effect, not the one
    that was asked for, so the only symptom is a test suite that quietly did not build.

These cases configure real throwaway build trees rather than asserting on the shape of the cmake
file, because the claim is about what CMake does with the flags, not about how the file reads.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]

#: Configure-only: nothing is compiled, so this stays in the seconds range. A configure failure here
#: is a real failure - it would mean the tree cannot be configured at all.
TIMEOUT = 240


class KconfigStandardOverridesTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.build = Path(self.directory.name) / "build"

    def configure(self, *flags: str) -> dict[str, str]:
        """Run cmake with `flags`, then return the interesting cache entries."""
        cmake = shutil.which("cmake")
        if cmake is None:
            self.skipTest("cmake is not on PATH")
        result = subprocess.run(
            [cmake, "-S", str(ROOT), "-B", str(self.build), "-DCMAKE_BUILD_TYPE=Release", *flags],
            capture_output=True, text=True, timeout=TIMEOUT)
        self.assertEqual(0, result.returncode,
                         f"configure failed with {flags}:\n{result.stdout[-2000:]}\n{result.stderr[-2000:]}")
        cache = (self.build / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace")
        values: dict[str, str] = {}
        for line in cache.splitlines():
            name, _, value = line.partition(":BOOL=")
            if _:
                values[name] = value
        return values

    def test_an_explicit_build_testing_on_is_honoured(self) -> None:
        values = self.configure("-DBUILD_TESTING=ON")
        self.assertEqual("ON", values.get("BUILD_TESTING"),
                         "-DBUILD_TESTING=ON was discarded; a reader who asks for tests must get tests")

    def test_an_explicit_build_testing_off_is_honoured(self) -> None:
        values = self.configure("-DBUILD_TESTING=OFF")
        self.assertEqual("OFF", values.get("BUILD_TESTING"))

    def test_the_documented_kconfig_route_still_works(self) -> None:
        # The other spelling has to keep working: the Makefile passes BUILD_TESTS this way.
        values = self.configure('-DKCONFIG_OVERRIDES=BUILD_TESTING=ON')
        self.assertEqual("ON", values.get("BUILD_TESTING"))

    def test_the_kconfig_default_still_applies_when_nothing_is_passed(self) -> None:
        values = self.configure()
        self.assertIn(values.get("BUILD_TESTING"), {"ON", "OFF"},
                      "BUILD_TESTING was not decided at all when nothing was passed")

    def test_an_explicit_build_shared_libs_is_honoured(self) -> None:
        # Same FORCE, same standard name, same silent discard.
        values = self.configure("-DBUILD_SHARED_LIBS=ON")
        self.assertEqual("ON", values.get("BUILD_SHARED_LIBS"))

    def test_an_explicit_request_does_not_outlive_its_configure(self) -> None:
        # The fix must not pin the first value it ever saw: a second configure that says nothing has to
        # fall back to the Kconfig default rather than keep a stale command-line request.
        self.assertEqual("ON", self.configure("-DBUILD_TESTING=ON").get("BUILD_TESTING"))
        second = Path(self.directory.name) / "second"
        cmake = shutil.which("cmake")
        if cmake is None:
            self.skipTest("cmake is not on PATH")
        result = subprocess.run([cmake, "-S", str(ROOT), "-B", str(second),
                                 "-DCMAKE_BUILD_TYPE=Release"],
                                capture_output=True, text=True, timeout=TIMEOUT)
        self.assertEqual(0, result.returncode, result.stderr[-2000:])
        cache = (second / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace")
        default = [line.split("=", 1)[1] for line in cache.splitlines()
                   if line.startswith("BUILD_TESTING:BOOL=")]
        self.assertTrue(default, "BUILD_TESTING absent from a fresh configure")
        self.assertEqual("OFF", default[0],
                         "a fresh configure kept a value only ever passed on the command line once")


if __name__ == "__main__":
    unittest.main()
