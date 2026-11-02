#!/usr/bin/env python3
"""Cases for the relative-link guard."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_doc_relative_links as guard  # noqa: E402


class LinkTests(unittest.TestCase):
    def build(self, tmp: str, files: dict[str, str]) -> None:
        for name, source in files.items():
            path = Path(tmp) / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(source, encoding="utf-8")

    def run_guard(self, page: str, source: str, extras: dict[str, str] | None = None) -> list[str]:
        """Judge one page inside a throwaway tree; ``extras`` are other files the tree contains."""
        with tempfile.TemporaryDirectory() as tmp:
            files = {page: source}
            files.update(extras or {})
            self.build(tmp, files)
            return guard.findings(Path(tmp), [Path(tmp) / page])

    # --- the claim itself -------------------------------------------------

    def test_a_link_to_a_sibling_page_passes(self):
        problems = self.run_guard("docs/index.md", "See [guide](guide.md).\n",
                                  {"docs/guide.md": "# Guide\n"})
        self.assertEqual([], problems)

    def test_a_link_that_does_not_exist_anywhere_is_reported(self):
        problems = self.run_guard("docs/index.md", "See [guide](guide.md).\n")
        self.assertEqual(1, len(problems))
        self.assertIn("docs/index.md:1", problems[0])
        self.assertIn("guide.md", problems[0])
        self.assertIn("404", problems[0])

    def test_a_link_written_from_the_repository_root_names_that_mistake(self):
        """The dominant defect: `components/hal/README.md` on a page one level down."""
        problems = self.run_guard(
            "docs/components/index.md", "- [HAL](components/hal/README.md)\n",
            {"components/hal/README.md": "# HAL\n"})
        self.assertEqual(1, len(problems))
        self.assertIn("repository root", problems[0])
        self.assertIn("docs/components/components/hal/README.md", problems[0])

    def test_a_correct_relative_path_is_not_reported_even_when_it_also_resolves_from_the_root(self):
        problems = self.run_guard("docs/index.md", "[u](getting-started/toolchain.md)\n",
                                  {"docs/getting-started/toolchain.md": "# t\n"})
        self.assertEqual([], problems)

    def test_a_parent_relative_path_passes(self):
        problems = self.run_guard("docs/components/hal/introduction.md", "[h](../../hal/HAL_UNIFIED_API.md)\n",
                                  {"docs/hal/HAL_UNIFIED_API.md": "# HAL\n"})
        self.assertEqual([], problems)

    # --- what is not a link ----------------------------------------------

    def test_a_link_inside_a_fenced_block_is_not_a_claim(self):
        """`CONTRIBUTING.md` prints this template for a contributor to copy."""
        source = "```markdown\n- [相关文档 1](link1.md)\n```\n"
        self.assertEqual([], self.run_guard("CONTRIBUTING.md", source))

    def test_a_link_inside_an_inline_code_span_is_not_a_claim(self):
        self.assertEqual([], self.run_guard("docs/index.md", "Write it as `[x](nowhere.md)`.\n"))

    def test_a_tilde_fence_is_treated_like_a_backtick_fence(self):
        source = "~~~markdown\n[x](nowhere.md)\n~~~\n"
        self.assertEqual([], self.run_guard("docs/index.md", source))

    def test_a_nested_fence_does_not_hide_the_rest_of_the_page(self):
        """`docs/index.md` prints a markdown template whose body is a ```c block."""
        source = ("```markdown\n## 示例\n```c\nint x;\n```\n\n[x](gone.md)\n")
        problems = self.run_guard("docs/index.md", source)
        self.assertEqual(1, len(problems))
        self.assertIn("docs/index.md:7", problems[0])

    def test_a_longer_fence_closes_and_a_shorter_one_does_not(self):
        source = "````markdown\n```\n[x](gone.md)\n```\n````\n"
        self.assertEqual([], self.run_guard("docs/index.md", source))

    def test_external_and_anchor_links_are_not_relative_claims(self):
        source = ("[a](https://example.com) [b](http://example.com) [c](mailto:x@y.z) "
                  "[d](#section) [e](ftp://x/y)\n")
        self.assertEqual([], self.run_guard("docs/index.md", source))

    def test_a_placeholder_target_is_a_name_still_being_filled_in(self):
        source = "[a](<name>.md) [b]({id}.md) [c](*.md) [d](path/...)\n"
        self.assertEqual([], self.run_guard("docs/index.md", source))

    # --- resolution details ----------------------------------------------

    def test_an_anchor_suffix_is_resolved_by_path(self):
        problems = self.run_guard("docs/index.md", "[x](guide.md#install)\n",
                                  {"docs/guide.md": "# Guide\n"})
        self.assertEqual([], problems)

    def test_a_percent_encoded_cjk_filename_resolves(self):
        """`docs/README.md` links a Chinese-titled note with its spaces percent-encoded."""
        problems = self.run_guard("docs/index.md", "[日志](misc/%E6%97%A5%E5%BF%97.md)\n",
                                  {"docs/misc/日志.md": "# 日志\n"})
        self.assertEqual([], problems)

    def test_a_directory_target_must_be_a_directory(self):
        problems = self.run_guard("docs/index.md", "[examples](examples/)\n")
        self.assertEqual(1, len(problems))
        self.assertEqual([], self.run_guard("docs/index.md", "[examples](examples/)\n",
                                            {"docs/examples/README.md": "# e\n"}))

    def test_an_image_link_is_judged_like_any_other(self):
        """An image that is not there renders as a broken box, which is the same defect."""
        problems = self.run_guard("docs/index.md", "![diagram](platform.drawio)\n")
        self.assertEqual(1, len(problems))
        self.assertIn("platform.drawio", problems[0])

    def test_a_link_title_is_not_part_of_the_target(self):
        problems = self.run_guard("docs/index.md", '[x](guide.md "The guide")\n',
                                  {"docs/guide.md": "# Guide\n"})
        self.assertEqual([], problems)

    def test_a_raw_space_in_the_destination_is_reported(self):
        """Markdown needs `%20` or `<...>`: `[x](6A 工作流.md)` renders as literal brackets."""
        problems = self.run_guard("docs/index.md", "[x](6A 工作流.md)\n",
                                  {"docs/6A 工作流.md": "# 6A\n"})
        self.assertEqual(1, len(problems))
        self.assertIn("raw space", problems[0])

    def test_a_space_wrapped_destination_is_accepted(self):
        problems = self.run_guard("docs/index.md", "[x](<6A 工作流.md>)\n",
                                  {"docs/6A 工作流.md": "# 6A\n"})
        self.assertEqual([], problems)

    def test_every_defect_on_a_page_is_reported_not_just_the_first(self):
        source = "[a](gone.md)\n[b](also-gone.md)\n"
        self.assertEqual(2, len(self.run_guard("docs/index.md", source)))

    # --- what the walk reads ---------------------------------------------

    def test_the_walk_skips_records_and_vendor_readmes_and_keeps_the_rest(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.build(tmp, {
                "README.md": "# root\n",
                "docs/index.md": "# index\n",
                "docs/getting-started/x.md": "# x\n",
                "docs/history/old.md": "# record\n",
                "docs/plans/plan.md": "# plan\n",
                "docs/design/today.md": "# design\n",
                "docs/design/decision-2026-08-12.md": "# dated record\n",
                "components/hal/README.md": "# hal\n",
                "components/third_party/vendor/README.md": "# vendor\n",
                "docs/rules/RULEBOOK.md": "# rules\n",
            })
            names = {path.relative_to(tmp).as_posix() for path in guard.documents(Path(tmp))}
        self.assertIn("README.md", names)
        self.assertIn("docs/index.md", names)
        self.assertIn("docs/getting-started/x.md", names)
        self.assertIn("docs/design/today.md", names)
        self.assertIn("components/hal/README.md", names)
        self.assertNotIn("docs/history/old.md", names)
        self.assertNotIn("docs/plans/plan.md", names)
        self.assertNotIn("docs/design/decision-2026-08-12.md", names)
        self.assertNotIn("components/third_party/vendor/README.md", names)
        self.assertNotIn("docs/rules/RULEBOOK.md", names)

    def test_an_empty_walk_is_a_finding_not_a_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(Path(tmp), [])
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_repository_itself_passes(self):
        """The tree this guard ships in, so a new dead link fails here before it fails the gate."""
        self.assertEqual([], guard.findings())

    # --- the site navigation ---------------------------------------------

    def test_a_nav_entry_naming_a_missing_page_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.build(tmp, {"docs/mkdocs.yml": "nav:\n  - 首页: index.md\n  - 构建系统: toolchain/build-system.md\n",
                             "docs/index.md": "# index\n"})
            problems = guard.nav_findings(Path(tmp))
        self.assertEqual(1, len(problems))
        self.assertIn("mkdocs.yml:3", problems[0])
        self.assertIn("toolchain/build-system.md", problems[0])

    def test_a_nav_entry_naming_a_real_page_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.build(tmp, {"docs/mkdocs.yml": "nav:\n  - 组件总览: components/index.md\n",
                             "docs/components/index.md": "# index\n"})
            self.assertEqual([], guard.nav_findings(Path(tmp)))

    def test_a_nav_group_header_is_not_an_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.build(tmp, {"docs/mkdocs.yml": "nav:\n  - 组件文档:\n    - 总览: components/index.md\n",
                             "docs/components/index.md": "# index\n"})
            self.assertEqual([], guard.nav_findings(Path(tmp)))

    def test_a_nav_that_names_nothing_is_a_finding(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.build(tmp, {"docs/mkdocs.yml": "nav: []\n"})
            problems = guard.nav_findings(Path(tmp))
        self.assertEqual(1, len(problems))
        self.assertIn("not parsed", problems[0])

    def test_the_repository_navigation_is_green(self):
        self.assertEqual([], guard.nav_findings())

    def test_main_runs_the_navigation_rule(self):
        """A rule the entry point forgets to call is a rule that never fails the gate."""
        with mock.patch.object(guard, "nav_findings", return_value=["sentinel"]) as called:
            self.assertEqual(1, guard.main())
        self.assertTrue(called.called)

    def test_the_walk_reaches_the_pages_that_instruct(self):
        """Scope shrinkage would otherwise pass silently: an unwatched page has no findings."""
        names = {path.resolve().relative_to(guard.ROOT.resolve()).as_posix()
                 for path in guard.documents()}
        for expected in ("docs/index.md", "docs/api/index.md", "docs/components/index.md",
                         "docs/hardware/porting.md", "docs/components/hal/introduction.md",
                         "docs/design/XinYi_Project_Overview.md", "README.md", "CONTRIBUTING.md"):
            self.assertIn(expected, names)


if __name__ == "__main__":
    unittest.main(verbosity=2)
