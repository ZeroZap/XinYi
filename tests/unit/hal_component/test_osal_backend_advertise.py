#!/usr/bin/env python3
"""Synthetic cases for the OSAL backend consistency guard.

The real tree passes, which on its own proves nothing about whether the guard can fail.
These cases pin both halves: the three-set comparison, and the rule that a copy-pasteable
command in docs/design must name a backend the build actually accepts.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_osal_backend_advertise as guard  # noqa: E402


ROOT_OK = """
if(CONFIG_OSAL_BACKEND_FREERTOS)
    set(OSAL_BACKEND "freertos" CACHE STRING "OSAL backend selection" FORCE)
elseif(CONFIG_OSAL_BACKEND_BAREMETAL)
    set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend selection" FORCE)
endif()
"""

ROOT_ONE = """
if(CONFIG_OSAL_BACKEND_BAREMETAL)
    set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend selection" FORCE)
endif()
"""

OSAL_OK = """
set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos)
if(OSAL_BACKEND STREQUAL "baremetal")
elseif(OSAL_BACKEND STREQUAL "freertos")
elseif(OSAL_BACKEND STREQUAL "rtthread")
endif()
"""

OSAL_OVER_ADVERTISING = """
set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos rtthread cmsis_rtx)
if(OSAL_BACKEND STREQUAL "baremetal")
elseif(OSAL_BACKEND STREQUAL "freertos")
elseif(OSAL_BACKEND STREQUAL "rtthread")
endif()
"""

OSAL_ORPHAN_BRANCH = """
set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos)
if(OSAL_BACKEND STREQUAL "baremetal")
elseif(OSAL_BACKEND STREQUAL "freertos")
elseif(OSAL_BACKEND STREQUAL "zephyr")
endif()
"""

UNREACHABLE = {"rtthread": "not selectable yet"}


class SetTests(unittest.TestCase):
    def run_guard(self, root, osal, unreachable=UNREACHABLE):
        return guard.findings(root, osal, unreachable)

    def test_matching_sets_pass(self):
        self.assertEqual(self.run_guard(ROOT_OK, OSAL_OK), [])

    def test_advertising_a_backend_the_root_cannot_select_is_reported(self):
        problems = self.run_guard(ROOT_OK, OSAL_OVER_ADVERTISING)
        self.assertEqual(len(problems), 2)
        self.assertTrue(any("cmsis_rtx" in problem for problem in problems))
        self.assertTrue(any("rtthread" in problem for problem in problems))

    def test_selectable_but_not_advertised_is_reported(self):
        # Only the missing advertisement here: the freertos branch still exists and is in
        # no advertised set either, which is a second real finding on top of the first.
        osal = ("set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal)\n"
                "if(OSAL_BACKEND STREQUAL \"baremetal\")\n"
                "elseif(OSAL_BACKEND STREQUAL \"freertos\")\nendif()\n")
        problems = self.run_guard(ROOT_OK, osal, {})
        self.assertEqual(len(problems), 2)
        self.assertTrue(any("selectable but not offered" in problem for problem in problems))
        self.assertTrue(any("dead code" in problem for problem in problems))

    def test_a_branch_outside_every_set_is_reported(self):
        problems = self.run_guard(ROOT_OK, OSAL_ORPHAN_BRANCH, {})
        self.assertEqual(len(problems), 1)
        self.assertIn("zephyr", problems[0])

    def test_reachable_branches_need_no_declaration(self):
        # freertos is offered and selectable, so no unreachable entry should be required.
        osal = OSAL_OK.replace('elseif(OSAL_BACKEND STREQUAL "rtthread")\n', "")
        self.assertEqual(self.run_guard(ROOT_OK, osal, {}), [])

    def test_a_stale_unreachable_declaration_is_reported(self):
        osal = OSAL_OK.replace('elseif(OSAL_BACKEND STREQUAL "rtthread")\n', "")
        problems = self.run_guard(ROOT_OK, osal)
        self.assertEqual(len(problems), 1)
        self.assertIn("stale", problems[0])

    def test_an_unreachable_declaration_needs_a_reason(self):
        problems = self.run_guard(ROOT_OK, OSAL_OK, {"rtthread": "  "})
        self.assertEqual(len(problems), 1)
        self.assertIn("without a reason", problems[0])

    def test_a_parser_that_finds_nothing_fails(self):
        problems = self.run_guard("", "")
        self.assertEqual(len(problems), 1)
        self.assertIn("could not parse", problems[0])

    def test_the_single_backend_root_is_read_correctly(self):
        # guards the parser against assuming two branches always exist
        self.assertEqual(guard.selectable(ROOT_ONE), {"baremetal"})


class DocumentTests(unittest.TestCase):
    def test_a_fenced_command_naming_an_unavailable_backend_is_reported(self):
        docs = {"docs/design/x.md": "```bash\ncmake .. -DOSAL_BACKEND=rtthread\n```\n"}
        problems = guard.doc_command_findings(docs, {"baremetal", "freertos"})
        self.assertEqual(len(problems), 1)
        self.assertIn("-DOSAL_BACKEND=rtthread", problems[0])

    def test_a_valid_fenced_command_passes(self):
        docs = {"docs/design/x.md": "```bash\ncmake .. -DOSAL_BACKEND=freertos\n```\n"}
        self.assertEqual(guard.doc_command_findings(docs, {"baremetal", "freertos"}), [])

    def test_prose_may_explain_what_is_unavailable(self):
        docs = {"docs/design/x.md":
                "直接传 `-DOSAL_BACKEND=rtthread` 会被拒绝，因为该后端不可达。\n"}
        self.assertEqual(guard.doc_command_findings(docs, {"baremetal", "freertos"}), [])


class RepositoryTests(unittest.TestCase):
    def test_the_repository_passes(self):
        root = guard.ROOT_CMAKE.read_text(encoding="utf-8")
        problems = guard.findings(root, guard.OSAL_CMAKE.read_text(encoding="utf-8"))
        problems += guard.doc_command_findings(guard.design_docs(),
                                               guard.selectable(root))
        self.assertEqual(problems, [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
