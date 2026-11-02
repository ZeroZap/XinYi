#!/usr/bin/env python3
"""A guard that has never failed is a guard whose red nobody has seen.

`check_doc_api_symbols.py` passes on this repository, which is the state in which it is
indistinguishable from a script that inspects nothing and prints `ok`. These cases drive the same
`findings()` against synthetic documents and a synthetic header index, so every rule it claims to
enforce has a red it produces.

The index is injected rather than read so the cases stay honest about what is being judged: the
question is what the checker does with a token, not whether this tree happens to declare it today.

One contract this file had to discover: "no fenced C block was found" is judged **across all
candidates**, not per document. A single prose-only or shell-only document is therefore not a clean
result - it is the same red as a broken walk. So the cases that are about *what a document's fence
contains* hand in a companion document that does contain a clean C block; the walk's non-emptiness is
then not what is being measured, and it is measured on its own below.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_doc_api_symbols as guard  # noqa: E402

#: What the real tree declares, shrunk to two names plus a header. Enough to tell "declared" from
#: "guessed at" without pinning the test to a header that may be renamed.
SYMBOLS = {"xy_hal_gpio_init", "xy_hal_gpio_write", "xy_hal_gpio_config_t"}
NAMES = {"xy_hal_gpio.h"}

#: A document that is unambiguously clean, used to keep the global walk non-empty.
WITNESS = '#include "xy_hal_gpio.h"\nxy_hal_gpio_init(0);\n'


def fenced(body: str, language: str = "c") -> str:
    return f"# Doc\n\n```{language}\n{body}\n```\n"


class DocApiSymbolsTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def judged(self, text: str, language: str = "c", witness: bool = True) -> list[str]:
        paths: list[Path] = []
        if witness:
            companion = self.root / "WITNESS.md"
            companion.write_text(fenced(WITNESS), encoding="utf-8")
            paths.append(companion)
        path = self.root / "SAMPLE.md"
        path.write_text(fenced(text, language), encoding="utf-8")
        paths.append(path)
        return guard.findings(paths, SYMBOLS, NAMES)

    def test_a_sample_that_only_uses_declared_names_passes(self) -> None:
        self.assertEqual([], self.judged(
            '#include "xy_hal_gpio.h"\n'
            "xy_hal_gpio_config_t config;\n"
            "xy_hal_gpio_init(&config);\n"
            "xy_hal_gpio_write(0, 1);\n"))

    def test_a_name_no_header_declares_is_reported(self) -> None:
        # The original defect: the sample taught `xy_hal_pin_init()`, which was never a function.
        problems = self.judged("xy_hal_pin_init(&config);\n")
        self.assertTrue(any("xy_hal_pin_init" in p and "no header declares" in p for p in problems),
                        problems)

    def test_an_unknown_name_is_offered_its_nearest_declaration(self) -> None:
        problems = self.judged("xy_hal_gpio_initt(&config);\n")
        self.assertTrue(any("did you mean xy_hal_gpio_init?" in p for p in problems), problems)

    def test_a_far_name_offers_no_hint_instead_of_a_bad_one(self) -> None:
        problems = self.judged("xy_os_thread_create(0);\n")
        self.assertTrue(any("xy_os_thread_create" in p for p in problems), problems)
        self.assertFalse(any("did you mean" in p for p in problems), problems)

    def test_an_include_that_is_not_a_header_here_is_reported(self) -> None:
        problems = self.judged('#include "xy_crypto.h"\n')
        self.assertTrue(any('xy_crypto.h' in p and "not a header in this repository" in p
                            for p in problems), problems)

    def test_an_include_of_a_header_that_exists_passes(self) -> None:
        self.assertEqual([], self.judged('#include "xy_hal_gpio.h"\n'))

    def test_a_name_the_block_defines_itself_passes(self) -> None:
        # A sample is allowed a helper of its own; judging that would punish a correct example.
        self.assertEqual([], self.judged(
            "#define xy_led_pin 13\n"
            "xy_hal_gpio_write(xy_led_pin, 1);\n"))

    def test_a_naming_pattern_is_not_a_call(self) -> None:
        # `HAL_Component_Design_Overview.md` writes `xy_hal_<peripheral>_<operation>()` to describe
        # how names are shaped. The token regex stopped at the `<` and reported `xy_hal_` as a call,
        # which is the same category error the path rule already answers with `PLACEHOLDERS`.
        self.assertEqual([], self.judged("xy_hal_<peripheral>_<operation>()\n"))

    def test_a_template_block_may_include_the_header_it_tells_you_to_create(self) -> None:
        # The file-structure template writes `@file module_name.c` and then includes the header that
        # file will have. Matching stems mean the include is a placeholder for the reader's module.
        self.assertEqual([], self.judged(
            "/**\n"
            " * @file module_name.c\n"
            " */\n"
            '#include "module_name.h"\n'))
        # ... but only the block's own file: any other missing header is still a broken claim.
        problems = self.judged('/**\n * @file module_name.c\n */\n#include "not_a_header.h"\n')
        self.assertTrue(any("not_a_header.h" in p for p in problems), problems)

    def test_a_shell_block_is_not_an_api_claim(self) -> None:
        # `make <target>` is a different rule, owned by a different guard. Judging shell blocks here
        # would report every build command as an undeclared symbol.
        self.assertEqual([], self.judged("xy_not_a_symbol anywhere here", language="bash"))

    def test_prose_outside_a_fence_is_not_judged(self) -> None:
        prose = self.root / "PROSE.md"
        prose.write_text("# Doc\n\nCall `xy_totally_made_up()` to begin.\n", encoding="utf-8")
        companion = self.root / "WITNESS.md"
        companion.write_text(fenced(WITNESS), encoding="utf-8")
        self.assertEqual([], guard.findings([prose, companion], SYMBOLS, NAMES))

    def test_a_document_opening_and_closing_unbalanced_is_still_walked(self) -> None:
        # An unclosed fence must not swallow the rest of the document silently.
        path = self.root / "UNCLOSED.md"
        path.write_text("```c\nxy_made_up_symbol();\n", encoding="utf-8")
        problems = guard.findings([path], SYMBOLS, NAMES)
        self.assertTrue(any("xy_made_up_symbol" in p for p in problems), problems)

    def test_a_walk_over_no_block_at_all_reports_itself_broken(self) -> None:
        # The failure this whole file exists to make impossible: inspecting nothing and printing ok.
        path = self.root / "EMPTY.md"
        path.write_text("# Doc\n\nNo fenced C here.\n", encoding="utf-8")
        problems = guard.findings([path], SYMBOLS, NAMES)
        self.assertTrue(any("the walk is broken" in p for p in problems), problems)

    def test_only_one_document_without_a_fence_still_reports_the_walk(self) -> None:
        # Stated so the global reading of the rule is pinned, not just true by accident.
        empty = self.root / "EMPTY.md"
        empty.write_text("# Doc\n\nNo fenced C here.\n", encoding="utf-8")
        problems = guard.findings([empty], SYMBOLS, NAMES)
        self.assertTrue(any("the walk is broken" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # A path is a claim too. Same shape as the symbol cases: what the rule does with a token, not
    # whether this tree happens to have the directory today.
    # ---------------------------------------------------------------------------------------------

    def judged_paths(self, text: str) -> list[str]:
        path = self.root / "LOCATIONS.md"
        path.write_text("# Doc\n\n" + text + "\n" + fenced(WITNESS), encoding="utf-8")
        return guard.findings([path], SYMBOLS, NAMES, root=self.root)

    def test_a_path_a_reader_can_follow_passes(self) -> None:
        (self.root / "examples" / "demo").mkdir(parents=True)
        self.assertEqual([], self.judged_paths("The sample is `examples/demo/`."))

    def test_a_path_no_reader_can_follow_is_reported(self) -> None:
        # The original defect: four of five reading-order links and three of four example rows
        # pointed at paths that had been moved one directory down.
        problems = self.judged_paths("| Sensor | `examples/sensor_demo/` | reads it |")
        self.assertTrue(any("examples/sensor_demo" in p and "does not exist" in p for p in problems),
                        problems)

    def test_a_line_that_asks_the_reader_to_create_the_path_is_not_a_claim(self) -> None:
        # "Create examples/my_first_app/main.c" is an instruction, not an assertion that it exists.
        self.assertEqual([], self.judged_paths("创建文件 `examples/my_first_app/main.c`："))

    def test_a_placeholder_is_not_a_path_yet(self) -> None:
        self.assertEqual([], self.judged_paths("Put it in `examples/<your_app>/main.c`."))

    def test_a_line_that_asserts_the_path_is_absent_is_not_a_claim(self) -> None:
        # `xinyi-component-quality-loop.md` says the repository **没有** `tests/CMakeLists.txt`.
        # That sentence is the opposite of a pointer: the reader is told not to look. Reporting it
        # would make an accurate statement about the tree impossible to write down.
        self.assertEqual([], self.judged_paths("当前仓库没有 `tests/CMakeLists.txt`。"))
        # ... and the loophole stays shut: a plain pointer at the same path is still a finding.
        problems = self.judged_paths("统一入口在 `tests/CMakeLists.txt`。")
        self.assertTrue(any("tests/CMakeLists.txt" in p for p in problems), problems)

    def test_a_sweep_that_finds_no_path_reports_itself_broken(self) -> None:
        # Same reasoning as the fence walk: inspecting nothing must not look like a clean result.
        documents = self.root / "docs" / "getting-started"
        documents.mkdir(parents=True)
        (documents / "QUICK_START.md").write_text(fenced(WITNESS), encoding="utf-8")
        saved_root, saved_roots = guard.ROOT, guard.DOC_ROOTS
        guard.ROOT, guard.DOC_ROOTS = self.root, ("docs/getting-started",)
        try:
            problems = guard.findings(symbols=SYMBOLS, names=NAMES)
        finally:
            guard.ROOT, guard.DOC_ROOTS = saved_root, saved_roots
        self.assertTrue(any("path walk is broken" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # The two rules that came with the HAL scope: vendor headers are real headers, and a design note
    # that is skipped is skipped *visibly*.
    # ---------------------------------------------------------------------------------------------

    def test_a_vendor_sdk_header_is_a_real_header_but_vouches_for_no_symbol(self) -> None:
        # A porting note legitimately includes `stm32l4xx_hal.h`; that header lives under MCU/, not
        # under components/. Reading it for symbols as well would let any SDK name stand in for an
        # `xy_*` declaration, which is exactly the direction that has to stay closed.
        vendor = self.root / "MCU" / "ST" / "Vendor"
        vendor.mkdir(parents=True)
        (vendor / "stm32l4xx_hal.h").write_text("void HAL_Delay(unsigned ms);\n"
                                                "void xy_vendor_only_symbol(void);\n",
                                                encoding="utf-8")
        project = self.root / "components" / "hal"
        project.mkdir(parents=True)
        (project / "xy_hal_hdr.h").write_text("void xy_hal_gpio_init(int p);\n", encoding="utf-8")

        saved_root = guard.ROOT
        guard.ROOT = self.root
        try:
            symbols, names = guard.header_index(roots=("components",),
                                                header_roots=("components", "MCU"))
        finally:
            guard.ROOT = saved_root
        self.assertIn("stm32l4xx_hal.h", names)
        self.assertIn("xy_hal_gpio_init", symbols)
        self.assertNotIn("xy_vendor_only_symbol", symbols)
        # ... and the rule that follows from it: including the vendor header is not a finding.
        path = self.root / "SAMPLE.md"
        path.write_text(fenced('#include "stm32l4xx_hal.h"\n'), encoding="utf-8")
        self.assertEqual([], guard.findings([path], symbols, names))

    def test_an_exempt_document_is_skipped_by_the_walk(self) -> None:
        # Scope grew to a directory that also holds pre-2.0 design notes. They are exempt with a
        # recorded reason; the point of this case is that the walk honours the list rather than
        # reporting them forever (or dropping the reason requirement silently).
        hal_dir = self.root / "components" / "hal"
        hal_dir.mkdir(parents=True)
        (hal_dir / "OLD.md").write_text(fenced("xy_hal_pin_init();\n"), encoding="utf-8")
        (hal_dir / "README.md").write_text(
            fenced(WITNESS) + "\nThe sample lives in `components/hal/README.md`.\n",
            encoding="utf-8")

        saved = guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT
        guard.ROOT, guard.DOC_ROOTS = self.root, ("components/hal",)
        guard.EXEMPT = {"components/hal/OLD.md": "pre-2.0 design note"}
        try:
            found = [p.name for p in guard.documents()]
            problems = guard.findings(symbols=SYMBOLS, names=NAMES)
        finally:
            guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = saved
        self.assertEqual(["README.md"], found)
        self.assertEqual([], problems)

    # ---------------------------------------------------------------------------------------------
    # The README sweep: every component front door is a usage claim, and nothing else under
    # `components/` is.
    # ---------------------------------------------------------------------------------------------

    def swept(self) -> list[str]:
        """Documents the real walk picks up, relative to a temporary root.

        `README_ROOTS` is deliberately left as the module defines it: a helper that re-states the
        root would keep passing with the sweep unwired, which is the failure the footprint test
        exists to catch.
        """
        saved = guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT
        guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = self.root, (), {}
        try:
            return [p.relative_to(self.root).as_posix() for p in guard.documents()]
        finally:
            guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = saved

    def test_a_component_readme_is_swept_and_a_design_record_next_to_it_is_not(self) -> None:
        # The defect this sweep was added for: nine component READMEs taught APIs that do not
        # exist. The plan beside them describes work in progress and must stay out of the sweep,
        # or the rule starts punishing the act of writing a plan.
        component = self.root / "components" / "ipc"
        component.mkdir(parents=True)
        (component / "README.md").write_text(fenced("xy_broker_init();\n"), encoding="utf-8")
        (component / "IPC_PLAN.md").write_text(fenced("xy_future_api();\n"), encoding="utf-8")
        self.assertEqual(["components/ipc/README.md"], self.swept())

    def test_an_imported_vendor_readme_is_not_judged(self) -> None:
        # `components/third_party/**` and the two net/at imports quote Espressif/AWS APIs by
        # design. Judging them would report correct vendor documentation forever.
        for vendor in ("third_party/flashdb", "net/at/AT-Command-V2",
                       "net/at/FreeRTOS-Cellular-Interface"):
            directory = self.root / "components" / vendor
            directory.mkdir(parents=True)
            (directory / "README.md").write_text(fenced("esp_at_send();\n"), encoding="utf-8")
        ours = self.root / "components" / "net"
        (ours / "README.md").write_text(fenced("xy_mqtt_init();\n"), encoding="utf-8")
        self.assertEqual(["components/net/README.md"], self.swept())

    def test_the_real_tree_sweeps_component_readmes(self) -> None:
        # A footprint check on the checked-in tree: the sweep must actually reach component front
        # doors, and must not reach vendor ones. A walk wired to nothing reports green forever.
        walked = [p.relative_to(guard.ROOT).as_posix() for p in guard.documents()
                  if p.relative_to(guard.ROOT).as_posix().startswith("components/")]
        readmes = [p for p in walked if p.endswith("README.md")]
        self.assertGreaterEqual(len(readmes), 20, walked)
        self.assertEqual([], [p for p in walked if not p.endswith("README.md")], walked)
        self.assertEqual([], [p for p in walked if "/third_party/" in p], walked)

    def test_the_real_tree_sweeps_the_curated_api_index(self) -> None:
        # The API index taught `xy_os_thread_create()` and `xy_os_thread_t` while this walk reported
        # `ok` about the directories next door. A footprint case is what keeps the scope from being
        # narrowed back to silence: if `docs/api` leaves DOC_ROOTS, this goes red.
        walked = [p.relative_to(guard.ROOT).as_posix() for p in guard.documents()]
        self.assertIn("docs/api/API_REFERENCE.md", walked, walked)

    def test_a_dated_design_record_is_skipped_and_an_undated_page_is_judged(self) -> None:
        # `docs/design` holds both answers to "what does the tree look like": the proposal that
        # decided something on a date, and the page that describes it now. The date in the filename
        # is the record's own banner, so the walk takes the undated page and leaves the other alone.
        design = self.root / "docs" / "design"
        design.mkdir(parents=True)
        (design / "LIVE.md").write_text(fenced("xy_dead_api();\n"), encoding="utf-8")
        (design / "decision-proposal-2026-08-11.md").write_text(fenced("xy_dead_api();\n"),
                                                                encoding="utf-8")
        saved = guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT
        guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = self.root, ("docs/design",), {}
        try:
            walked = [p.name for p in guard.documents()]
        finally:
            guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = saved
        self.assertEqual(["LIVE.md"], walked)

    def test_the_real_tree_sweeps_the_live_design_pages_and_not_the_dated_records(self) -> None:
        # Footprint on the checked-in tree: `docs/design` is where the current design lives, so the
        # undated pages must be in the walk, and the thirty-two dated decisions must not be. If the
        # root leaves DOC_ROOTS the first assertion goes red; if the record rule stops working the
        # second one does.
        walked = [p.relative_to(guard.ROOT).as_posix() for p in guard.documents()]
        design = [p for p in walked if p.startswith("docs/design/")]
        self.assertIn("docs/design/XinYi_Project_Overview.md", design, design)
        self.assertEqual([], [p for p in design if guard.DATED_RECORD.search(p)], design)
        self.assertGreaterEqual(len(design), 10, design)

    # ---------------------------------------------------------------------------------------------
    # A path in a sentence about the past is history, not a pointer - but only the verb makes it so.
    # ---------------------------------------------------------------------------------------------

    def test_a_line_recording_that_a_path_was_removed_is_history(self) -> None:
        # The charger README says the old `components/charger/` component "已在 ... 移除". A reader
        # is not being sent there; reporting it punishes an accurate record of the migration.
        self.assertEqual([], self.judged_paths(
            "旧 `components/charger/` compatibility component 已在迁移后移除。"))

    def test_a_line_recording_that_a_path_was_merged_is_history(self) -> None:
        # `test_layout_analysis.md` records that `test_filter.c` **已并入** `test_clib_core.c`.
        # A merge is a removal with a destination; reporting it punishes the accurate record.
        self.assertEqual([], self.judged_paths(
            "- `components/clib/xy_clib/test_filter.c` 已并入 `tests/unit/clib/test_clib_core.c`"))

    def test_a_bare_mention_of_an_old_path_still_fails(self) -> None:
        # The loophole that has to stay shut: calling a missing directory "legacy" is not a
        # removal record, and a reader can still try to open what the sentence points at.
        problems = self.judged_paths("旧 `components/charger/` 是上一代实现，细节见该目录。")
        self.assertTrue(any("components/charger" in p and "does not exist" in p for p in problems),
                        problems)



    # ---------------------------------------------------------------------------------------------
    # Skipping a document is the one move that can make this walk lie. An exemption carries two
    # obligations: the page must tell the reader it is a record, and the entry must name a file.
    # ---------------------------------------------------------------------------------------------

    def exempted(self, body: str, relative: str = "components/hal/OLD.md") -> list[str]:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        return guard.exemption_findings(root=self.root,
                                         exempt={relative: "pre-2.0 design note"})

    def test_an_exempt_document_that_does_not_announce_itself_is_reported(self) -> None:
        # Measured before this rule: all seven opened with their original claim - one with
        # "**状态**: ✅ 完成" - so the only pages in the sweep that still teach `xy_hal_pin_init()`
        # were also the only pages with no warning on them.
        problems = self.exempted(fenced("xy_hal_pin_init();\n"))
        self.assertTrue(any("does not open with" in p and "OLD.md" in p for p in problems), problems)

    def test_the_banner_makes_the_exemption_visible_and_the_list_clean(self) -> None:
        problems = self.exempted(guard.EXEMPT_BANNER + "\n\n" + fenced("xy_hal_pin_init();\n"))
        self.assertEqual([], problems)

    def test_a_banner_below_the_reading_window_does_not_count(self) -> None:
        # A footnote satisfies a substring search and informs nobody: the reader has already taken
        # in the old API before being told it never existed.
        body = ("# Old\n\n"
                + "\n".join(f"line {n}" for n in range(guard.EXEMPT_BANNER_WINDOW + 5))
                + "\n\n" + guard.EXEMPT_BANNER + "\n")
        problems = self.exempted(body)
        self.assertTrue(any("does not open with" in p for p in problems), problems)

    def test_an_exemption_for_a_file_that_is_gone_is_reported(self) -> None:
        problems = guard.exemption_findings(root=self.root,
                                            exempt={"components/hal/GONE.md": "deleted doc"})
        self.assertTrue(any("GONE.md" in p and "not a file in this tree" in p for p in problems),
                        problems)

    def test_judge_carries_the_exemption_rule_not_just_the_document_rules(self) -> None:
        # The wiring case: a rule `main()` never calls is decoration. `judge()` is what `main()`
        # reports, so driving it here is what puts the exemption list inside the gate.
        doc = self.root / "components" / "hal" / "OLD.md"
        doc.parent.mkdir(parents=True)
        doc.write_text(fenced("xy_hal_pin_init();\n"), encoding="utf-8")
        saved = guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT
        guard.ROOT, guard.DOC_ROOTS = self.root, ("components/hal",)
        guard.EXEMPT = {"components/hal/OLD.md": "pre-2.0 design note"}
        try:
            problems = guard.judge(SYMBOLS, NAMES)
        finally:
            guard.ROOT, guard.DOC_ROOTS, guard.EXEMPT = saved
        self.assertTrue(any("does not open with" in p for p in problems), problems)

    def test_the_real_exemption_list_is_announced_by_the_real_documents(self) -> None:
        # Footprint on the checked-in tree: an eighth record exempted without a banner, or one of
        # the seven deleted and left on the list, goes red here.
        self.assertGreaterEqual(len(guard.EXEMPT), 7)
        self.assertEqual([], guard.exemption_findings())


if __name__ == "__main__":
    unittest.main()
