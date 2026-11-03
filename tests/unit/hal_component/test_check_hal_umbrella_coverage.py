#!/usr/bin/env python3
"""The umbrella guard's rules, each driven to a red that was actually produced.

`check_hal_umbrella_coverage.py` prints `22 peripheral header(s), 22/22 exported` on this repository,
which is also what a script with a hard-coded pair of numbers would print. These cases run the same
`findings()` against throwaway trees so every rule has a failure it can be seen to raise:

* a peripheral header that the umbrella does not export;
* an umbrella include that names a header which does not exist;
* an include that reaches outside `components/hal/inc`;
* the same header included twice;
* an exemption with a reason (accepted), without one, and one whose header is gone;
* an exemption that contradicts the umbrella;
* a missing directory, a missing umbrella, and an umbrella that exports nothing at all.

The trees are synthetic and two or three headers wide on purpose: the question is what the checker
does with a header nobody exported, not how many peripherals today's `inc/` happens to hold.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_hal_umbrella_coverage as guard  # noqa: E402

HEADER = """\
#ifndef {guard}
#define {guard}
#endif
"""

UMBRELLA = """\
#ifndef XY_HAL_H
#define XY_HAL_H
{includes}#endif /* XY_HAL_H */
"""


def umbrella(*names: str) -> str:
    return UMBRELLA.format(includes="".join(f'#include "{name}"\n' for name in names))


class HalUmbrellaCoverageTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.inc = self.root / "components" / "hal" / "inc"
        self.inc.mkdir(parents=True)
        self.write("xy_hal_gpio.h")
        self.write("xy_hal_dac.h")
        self.write("xy_hal.h", umbrella("xy_hal_error.h", "xy_hal_dac.h", "xy_hal_gpio.h"))
        self.write("xy_hal_error.h")

    def write(self, name: str, text: str | None = None) -> None:
        path = self.inc / name
        path.write_text(HEADER.format(guard=name.upper().replace(".", "_")) if text is None else text)

    def test_complete_tree_reports_no_findings(self) -> None:
        self.assertEqual([], guard.findings(self.root))
        # gpio, dac and error are all `xy_hal_*.h` in this tree, and all three are exported.
        self.assertIn("3/3 exported", guard.report(self.root))

    def test_unexported_header_is_a_finding(self) -> None:
        self.write("xy_hal_flash.h")
        problems = guard.findings(self.root)
        self.assertEqual(1, len(problems), problems)
        self.assertIn("does not export xy_hal_flash.h", problems[0])

    def test_dangling_include_is_a_finding(self) -> None:
        self.write("xy_hal.h", umbrella("xy_hal_gpio.h", "xy_hal_ghost.h"))
        problems = guard.findings(self.root)
        self.assertTrue(any("xy_hal_ghost.h, which does not exist" in item for item in problems),
                        problems)

    def test_include_outside_the_directory_is_a_finding(self) -> None:
        self.write("xy_hal.h", umbrella("xy_hal_gpio.h", "xy_hal_dac.h", "../../xy_os.h"))
        problems = guard.findings(self.root)
        self.assertTrue(any("not a header of this directory" in item for item in problems), problems)

    def test_duplicate_include_is_a_finding(self) -> None:
        self.write("xy_hal.h", umbrella("xy_hal_gpio.h", "xy_hal_gpio.h", "xy_hal_dac.h"))
        problems = guard.findings(self.root)
        self.assertTrue(any("includes xy_hal_gpio.h twice" in item for item in problems), problems)

    def test_exemption_with_a_reason_is_accepted(self) -> None:
        self.write("xy_hal_flash.h")
        problems = guard.findings(self.root, exempt={"xy_hal_flash.h": "backend-only in this tree"})
        self.assertEqual([], problems)

    def test_exemption_without_a_reason_is_a_finding(self) -> None:
        self.write("xy_hal_flash.h")
        problems = guard.findings(self.root, exempt={"xy_hal_flash.h": "  "})
        self.assertTrue(any("without a reason" in item for item in problems), problems)

    def test_exemption_of_a_deleted_header_is_a_finding(self) -> None:
        problems = guard.findings(self.root, exempt={"xy_hal_ghost.h": "was backend-only"})
        self.assertTrue(any("the header is gone" in item for item in problems), problems)

    def test_exempt_yet_exported_is_a_finding(self) -> None:
        problems = guard.findings(self.root, exempt={"xy_hal_dac.h": "backend-only"})
        self.assertTrue(any("yet xy_hal.h includes it" in item for item in problems), problems)

    def test_empty_directory_is_a_finding(self) -> None:
        for name in ("xy_hal_gpio.h", "xy_hal_dac.h", "xy_hal_error.h"):
            (self.inc / name).unlink()
        problems = guard.findings(self.root)
        self.assertTrue(any("no `xy_hal_<peripheral>.h` was found" in item for item in problems),
                        problems)

    def test_missing_directory_is_a_finding(self) -> None:
        self.inc.rename(self.root / "elsewhere")
        problems = guard.findings(self.root)
        self.assertEqual(1, len(problems), problems)
        self.assertIn("header directory is not there", problems[0])

    def test_missing_umbrella_is_a_finding(self) -> None:
        (self.inc / "xy_hal.h").unlink()
        problems = guard.findings(self.root)
        self.assertEqual(1, len(problems), problems)
        self.assertIn("the front door is gone", problems[0])

    def test_real_tree_exports_every_header(self) -> None:
        problems = guard.findings()
        self.assertEqual([], problems)
        exported = guard.report()
        self.assertTrue(exported.startswith("hal_umbrella ok ("), exported)
        total = exported.split("(")[1].split(" ")[0]
        self.assertIn(f"{total}/{total} exported", exported)


if __name__ == "__main__":
    unittest.main()
