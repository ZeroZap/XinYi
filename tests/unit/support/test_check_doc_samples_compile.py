#!/usr/bin/env python3
"""A guard whose red nobody has seen is a guard that might be green for the wrong reason.

`check_doc_samples_compile.py` passes on this repository - the state in which it looks exactly like a
script that reads nothing and prints `ok`. These cases drive the same `findings()` against throwaway
documents, with the real compiler and the real headers, so every rule it claims to enforce has a red
that was actually produced.

The documents are synthetic and small on purpose. The question is what the checker does with a
prototype that disagrees with its header, not whether today's `API_REFERENCE.md` happens to be right -
that is the CTest's job, and pinning it here would make a documentation edit look like a test failure.

The include roots point at this repository's headers on purpose too: a stub header invented here would
prove the compiler runs without proving the guard reads the tree being judged.
"""

from __future__ import annotations

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_doc_samples_compile as guard  # noqa: E402

ROOT = guard.ROOT
#: Enough of the HAL to declare `xy_hal_gpio_init()` the way the header does.
HAL_ROOTS = (
    (ROOT / "components/hal/inc").as_posix(),
    (ROOT / "components/clib/xy_clib/inc").as_posix(),
    (ROOT / "components/clib/xy_clib").as_posix(),
    (ROOT / "components/common/inc").as_posix(),
)
HAL_INCLUDES = ("xy_hal_gpio.h",)
CORRECT = ("xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,\n"
           "                                const xy_hal_gpio_config_t *config);\n")


def fenced(body: str, language: str = "c") -> str:
    return f"# Doc\n\n```{language}\n{body}\n```\n"


class DocSamplesCompileTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.compiler = shutil.which("cc") or shutil.which("gcc")
        if not self.compiler:
            self.skipTest("no C compiler on PATH - the guard itself reports this as a finding")

    def judged(self, body: str, language: str = "c",
               includes: tuple[str, ...] = HAL_INCLUDES,
               roots: tuple[str, ...] = HAL_ROOTS) -> list[str]:
        doc = self.root / "SAMPLE.md"
        doc.write_text(fenced(body, language), encoding="utf-8")
        sample = guard.Sample(doc="SAMPLE.md", includes=includes, roots=roots)
        return guard.findings(samples=(sample,), root=self.root, compiler=self.compiler)

    # ---------------------------------------------------------------------------------------------
    # The defect that started this: a prototype the header contradicts.
    # ---------------------------------------------------------------------------------------------

    def test_a_prototype_that_matches_its_header_compiles(self) -> None:
        self.assertEqual([], self.judged(CORRECT))

    def test_a_parameter_of_the_wrong_width_is_reported(self) -> None:
        # Measured on the real page: `uint16_t pin` where the header takes `uint8_t`, so a reader
        # writing 0..15 by the page's type is not what the interface accepts.
        problems = self.judged("xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port,\n"
                              "                                uint16_t pin,\n"
                              "                                const xy_hal_gpio_config_t *config);\n")
        self.assertTrue(any("do not compile" in p and "SAMPLE.md" in p for p in problems), problems)

    def test_an_extra_parameter_is_reported(self) -> None:
        # Measured on the real page: `xy_sht30_init(dev, i2c_handle, addr)` - three parameters, and
        # the function has two. `addr` belongs to `xy_sht30_init_addr()`.
        problems = self.judged(CORRECT.replace(
            "xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,\n"
            "                                const xy_hal_gpio_config_t *config)",
            "xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin, uint16_t extra,\n"
            "                                const xy_hal_gpio_config_t *config)"))
        self.assertTrue(any("do not compile" in p for p in problems), problems)

    def test_a_return_type_that_disagrees_with_the_enum_is_reported(self) -> None:
        # `-Wenum-int-mismatch` is a warning by default; without `-Werror` this page's
        # `int xy_os_kernel_start(void);` against `xy_os_status_t` would have scrolled past.
        problems = self.judged("int xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,\n"
                              "                      const xy_hal_gpio_config_t *config);\n")
        self.assertTrue(any("do not compile" in p for p in problems), problems)

    def test_a_type_no_header_defines_is_reported(self) -> None:
        problems = self.judged("xy_hal_pin_config_t config;\n")
        self.assertTrue(any("do not compile" in p for p in problems), problems)

    def test_the_diagnostic_names_the_page_and_the_page_s_line(self) -> None:
        # A temporary file's line number is a location nobody can open. `#line` is what makes the
        # report actionable: the caret lands on the offending line of the document itself.
        body = "\n\n" + "uint16_t pin;\nxy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port,\n" \
                        "                                uint16_t pin,\n" \
                        "                                const xy_hal_gpio_config_t *config);\n"
        problems = self.judged(body)
        self.assertTrue(any("SAMPLE.md:7" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # The shapes that would otherwise read as a pass.
    # ---------------------------------------------------------------------------------------------

    def test_a_document_with_no_c_block_reports_the_extractor(self) -> None:
        problems = self.judged("# no code here\n", language="markdown")
        self.assertTrue(any("no fenced C block was found" in p for p in problems), problems)

    def test_a_shell_block_is_not_compiled(self) -> None:
        # `make test-unit` in a shell block is a build instruction, not a signature; compiling it
        # would report correct documentation as broken.
        problems = self.judged("# not C at all\nmake test-unit\n", language="bash")
        self.assertTrue(any("no fenced C block was found" in p for p in problems), problems)

    def test_a_document_that_is_gone_reports_a_drifted_list(self) -> None:
        sample = guard.Sample(doc="GONE.md", includes=HAL_INCLUDES, roots=HAL_ROOTS)
        problems = guard.findings(samples=(sample,), root=self.root, compiler=self.compiler)
        self.assertTrue(any("GONE.md" in p and "is gone" in p for p in problems), problems)

    def test_a_compiler_that_cannot_run_is_reported_not_skipped(self) -> None:
        # A missing toolchain must not read as a clean document: that is the same shape as an empty
        # walk, and it is the failure mode a CI image without gcc would hit.
        doc = self.root / "SAMPLE.md"
        doc.write_text(fenced(CORRECT), encoding="utf-8")
        sample = guard.Sample(doc="SAMPLE.md", includes=HAL_INCLUDES, roots=HAL_ROOTS)
        problems = guard.findings(samples=(sample,), root=self.root, compiler="/nonexistent/cc")
        self.assertTrue(any("cannot run" in p for p in problems), problems)

    def test_a_walk_over_no_sample_reports_itself_broken(self) -> None:
        problems = guard.findings(samples=(), root=self.root, compiler=self.compiler)
        self.assertTrue(any("the walk is broken" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # A page may carry patterns beside its sample. The selector is content-anchored, because a line
    # number would land on whatever an earlier edit shifted it onto.
    # ---------------------------------------------------------------------------------------------

    def judged_marked(self, body: str, markers: tuple[str, ...],
                      fragments: tuple[str, ...] = ()) -> list[str]:
        doc = self.root / "SAMPLE.md"
        doc.write_text(body, encoding="utf-8")
        sample = guard.Sample(doc="SAMPLE.md", includes=HAL_INCLUDES, roots=HAL_ROOTS,
                              markers=markers, fragments=fragments)
        return guard.findings(samples=(sample,), root=self.root, compiler=self.compiler)

    def test_a_selective_page_judges_its_sample_and_skips_the_patterns(self) -> None:
        # Same shape as the project overview: a template block that cannot compile at file scope,
        # then the usage sample. With the selector set, only the sample is compiled - and the
        # template is named as a fragment, because an unmatched block is a block nobody reads.
        page = (fenced("xy_hal_pin_config_t template_only;\n")
                + fenced("/* ---- the sample ---- */\n"
                         + CORRECT
                         + "void use(void) { xy_hal_gpio_write(0, 1, 1); }\n"))
        self.assertEqual([], self.judged_marked(page, ("---- the sample ----",),
                                                fragments=("template_only",)))

    def test_a_selector_that_matches_nothing_is_reported(self) -> None:
        # The selector has to be able to fail, or a renamed marker would quietly compile nothing and
        # the page would look as clean as a correct one.
        problems = self.judged_marked(fenced(CORRECT), ("a marker that is not in the page",))
        self.assertTrue(any("the selector no longer points" in p for p in problems), problems)

    def test_one_stale_marker_of_several_is_reported(self) -> None:
        # The OR selector's quiet failure: two markers, one renamed, and the surviving one keeps the
        # page compiling half its samples. Measured on `porting.md`, whose naming contract and
        # functional demo are selected separately - dropping either must not read as a pass.
        page = fenced("/* ---- one ---- */\n" + CORRECT) + fenced("/* ---- two ---- */\nint two;\n")
        problems = self.judged_marked(page, ("---- one ----", "---- three ----"))
        self.assertTrue(any("---- three ----" in p and "no longer being compiled" in p
                            for p in problems), problems)
        # The block nobody now selects is a second finding, not a third one: it is read by nothing,
        # which is exactly the state the stale marker would otherwise hide.
        self.assertTrue(any("neither compiled nor declared a fragment" in p for p in problems),
                        problems)
        self.assertEqual(2, len(problems), problems)

    def test_the_selected_sample_is_still_judged(self) -> None:
        # ... and the selector must not turn into an exemption: a bad prototype in the selected block
        # is still a finding.
        page = (fenced("plain_fragment_at_file_scope;\n")
                + fenced("/* ---- the sample ---- */\n"
                         "xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint16_t pin,\n"
                         "                                const xy_hal_gpio_config_t *config);\n"))
        problems = self.judged_marked(page, ("---- the sample ----",),
                                     fragments=("plain_fragment_at_file_scope;",))
        self.assertTrue(any("do not compile" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # A component page is a set of complete programs, and its non-samples have to be named.
    # ---------------------------------------------------------------------------------------------

    def judged_page(self, body: str, markers: tuple[str, ...] = (),
                    fragments: tuple[str, ...] = (), per_block: bool = True,
                    as_body: bool = False, application: bool = False) -> list[str]:
        doc = self.root / "SAMPLE.md"
        doc.write_text(body, encoding="utf-8")
        sample = guard.Sample(doc="SAMPLE.md", includes=HAL_INCLUDES, roots=HAL_ROOTS,
                              markers=markers, per_block=per_block, fragments=fragments,
                              as_body=as_body, application=application)
        return guard.findings(samples=(sample,), root=self.root, compiler=self.compiler)

    def test_two_programs_on_one_page_need_per_block(self) -> None:
        # `docs/components/osal/introduction.md` carries an initialisation program and a task
        # program, each with its own `main()`. Merged into one unit the second `main()` is a
        # duplicate definition - a defect of the fixture, not of the page - so the page declares
        # `per_block` and each program is judged on its own.
        page = fenced(CORRECT + "int main(void) { return 0; }\n") + \
               fenced(CORRECT + "int main(void) { return 0; }\n")
        self.assertEqual([], self.judged_page(page))
        self.assertTrue(any("do not compile" in p for p in self.judged_page(page, per_block=False)),
                        "the merged mode must still report the duplicate definition it cannot judge")

    def test_per_block_names_the_block_that_fails(self) -> None:
        good = fenced(CORRECT + "int main(void) { return 0; }\n")
        bad = fenced("xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint16_t pin,\n"
                     "                                const xy_hal_gpio_config_t *config);\n"
                     "int main(void) { return 0; }\n")
        problems = self.judged_page(good + bad)
        # The line is the page's own line, not the temporary unit's: the reader has to be able to
        # open the document at the block that failed.
        starts = [start for start, _ in guard.code_blocks(self.root / "SAMPLE.md")]
        self.assertTrue(any(f"block at line {starts[1]}" in p for p in problems), problems)
        self.assertFalse(any(f"block at line {starts[0]}" in p for p in problems), problems)

    def test_a_block_no_selector_matches_is_reported_not_skipped(self) -> None:
        # The quiet failure: a page grows a third sample, the selectors still match the first two,
        # and the guard reports the page clean while the new block is read by nobody.
        page = (fenced("/* ---- one ---- */\n" + CORRECT + "void one(void) {}\n")
                + fenced("/* ---- two ---- */\n" + CORRECT + "void two(void) {}\n")
                + fenced("/* ---- three ---- */\nxy_hal_pin_config_t unread;\n"))
        problems = self.judged_page(page, markers=("---- one ----", "---- two ----"))
        self.assertTrue(any("neither compiled nor declared a fragment" in p for p in problems),
                        problems)

    def test_a_declared_fragment_is_not_compiled(self) -> None:
        # A `xy_xxx.h` template names types that exist only in the reader's own driver, and a
        # configuration preamble redefines the header's macros. Both are patterns; naming them is
        # what keeps them from being silently unread.
        page = (fenced("/* ---- template ---- */\nxy_hal_pin_config_t template_name;\n")
                + fenced("/* ---- sample ---- */\n" + CORRECT + "void use(void) {}\n"))
        self.assertEqual([], self.judged_page(page, fragments=("---- template ----",)))

    def test_a_fragment_marker_that_matches_nothing_is_reported(self) -> None:
        page = (fenced("/* ---- template ---- */\nxy_hal_pin_config_t template_name;\n")
                + fenced("/* ---- sample ---- */\n" + CORRECT + "void use(void) {}\n"))
        problems = self.judged_page(page, fragments=("---- removed ----",))
        self.assertTrue(any("names no block on the page" in p for p in problems), problems)

    def test_one_block_cannot_be_both_a_sample_and_a_fragment(self) -> None:
        # Two claims about one block is an ambiguity, not a precedence rule: whichever way it was
        # resolved, one of the two statements would be false while the guard printed ok.
        page = fenced("/* ---- sample ---- */\n" + CORRECT + "void use(void) {}\n")
        problems = self.judged_page(page, markers=("---- sample ----",),
                                    fragments=("---- sample ----",))
        self.assertTrue(any("both compiled and declared a fragment" in p for p in problems),
                        problems)

    def test_a_block_that_is_statements_compiles_as_a_body(self) -> None:
        # `xy_hal_gpio_toggle(port, pin);` at file scope is a syntax error about the statement, not
        # about the call. `as_body` wraps the block, hoists its own `#include`, and keeps the page's
        # line numbers by issuing `#line` after the wrapper.
        page = fenced('// 用法\n#include "xy_hal_gpio.h"\n\nxy_hal_gpio_toggle(0, 5);\n')
        self.assertEqual([], self.judged_page(page, as_body=True))
        self.assertTrue(any("do not compile" in p for p in self.judged_page(page, as_body=False)),
                        "at file scope the same block must still be reported")

    def test_a_body_wrapper_keeps_the_page_s_line_number(self) -> None:
        page = fenced('// 用法\n#include "xy_hal_gpio.h"\n\nxy_hal_gpio_pin_config_t bad;\n')
        problems = self.judged_page(page, as_body=True)
        # The declaration is the fourth line of the block, and the wrapper must not shift it: the
        # diagnostic has to name the line the reader can open, not the line the guard generated.
        lines = page.splitlines()
        expected = next(number for number, line in enumerate(lines, 1) if "pin_config_t bad" in line)
        self.assertTrue(any(f"SAMPLE.md:{expected}" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # An application page calls the reader's own code; the project's namespace is still a promise.
    # ---------------------------------------------------------------------------------------------

    def test_an_application_page_may_call_the_reader_s_own_function(self) -> None:
        # `led_toggle()` is the reader's; a page teaching a task body cannot declare it for them.
        page = fenced('void task(void *arg) {\n    (void)arg;\n    led_toggle();\n}\n')
        self.assertEqual([], self.judged_page(page, application=True))
        self.assertTrue(any("do not compile" in p for p in self.judged_page(page, application=False)),
                        "without the tolerance the same block is an error")

    def test_an_application_page_may_not_call_an_undeclared_project_function(self) -> None:
        # The carve-out that keeps the tolerance honest: `xy_` is a call into this tree, so its name
        # has to come from a header. Measured shape - `xy_hal_delay_ms()` compiled only as a warning
        # until the page included `xy_hal_delay.h`.
        page = fenced('void task(void *arg) {\n    (void)arg;\n    xy_delay_that_does_not_exist(10);\n}\n')
        problems = self.judged_page(page, application=True)
        self.assertTrue(any("teaches an API that does not exist" in p for p in problems), problems)
        self.assertTrue(any("xy_delay_that_does_not_exist" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # The remainder: a page is judged, or deferred with a reason, or reported.
    # ---------------------------------------------------------------------------------------------

    def test_a_component_page_in_neither_list_is_reported(self) -> None:
        page = self.root / "docs" / "components" / "unlisted" / "introduction.md"
        page.parent.mkdir(parents=True)
        page.write_text(fenced(CORRECT), encoding="utf-8")
        problems = guard.unlisted_pages(self.root)
        self.assertTrue(any("docs/components/unlisted/introduction.md" in p for p in problems),
                        problems)

    def test_a_deferral_without_a_reason_is_reported(self) -> None:
        saved = guard.DEFERRED
        try:
            guard.DEFERRED = (("docs/components/addc/introduction.md", "  "),)
            problems = guard.unlisted_pages(ROOT)
        finally:
            guard.DEFERRED = saved
        self.assertTrue(any("deferred without a reason" in p for p in problems), problems)

    def test_a_deferral_of_a_page_that_is_gone_is_reported(self) -> None:
        saved = guard.DEFERRED
        try:
            guard.DEFERRED = (("docs/components/gone/introduction.md", "whatever"),)
            problems = guard.unlisted_pages(ROOT)
        finally:
            guard.DEFERRED = saved
        self.assertTrue(any("the file is gone" in p for p in problems), problems)

    # ---------------------------------------------------------------------------------------------
    # Footprint on the checked-in tree: the list must still name a page that exists, and the page
    # must still be the page being judged.
    # ---------------------------------------------------------------------------------------------

    def test_the_real_list_names_the_api_reference(self) -> None:
        docs = [sample.doc for sample in guard.SAMPLES]
        self.assertIn("docs/api/API_REFERENCE.md", docs)
        self.assertEqual([], [doc for doc in docs if not (ROOT / doc).is_file()])

    def test_the_real_list_names_the_project_overview_sample(self) -> None:
        # The page taught `xy_hal_uart_open()`. The name sweep now sees the names; this case is what
        # keeps the *signature* of the corrected sample inside a compiler's reach.
        docs = [sample.doc for sample in guard.SAMPLES]
        self.assertIn("docs/design/XinYi_Project_Overview.md", docs)
        sample = [s for s in guard.SAMPLES if s.doc == "docs/design/XinYi_Project_Overview.md"][0]
        self.assertTrue(sample.markers, "the overview page must select its sample by content")
        self.assertEqual([], [root for root in sample.roots if not (ROOT / root).is_dir()])

    def test_the_real_list_names_the_component_guides(self) -> None:
        # The sixteen pages that carry programs, of which fourteen are judged here and two are the
        # measured remainder. Both facts are asserted: a page silently leaving the list is the defect
        # this guard exists to prevent, and a deferral without a reason is the same silence.
        docs = [sample.doc for sample in guard.SAMPLES]
        self.assertIn("docs/components/osal/introduction.md", docs)
        self.assertIn("docs/components/hal/introduction.md", docs)
        self.assertEqual([], [doc for doc in docs if not (ROOT / doc).is_file()])
        judged = set(docs) | {doc for doc, _ in guard.DEFERRED}
        self.assertEqual([], guard.unlisted_pages(ROOT))
        self.assertTrue(judged, "the component guide list is empty")

    def test_the_judged_component_pages_compile_every_block_they_claim(self) -> None:
        # The count is the claim. Fourteen pages carry forty-eight compiled blocks; a page that loses
        # its selector or its entry without the file changing would show up here as a shrinking total.
        blocks = sum(len(guard.code_blocks(ROOT / sample.doc)) for sample in guard.SAMPLES
                     if sample.doc.startswith("docs/components/"))
        self.assertGreaterEqual(blocks, 45, blocks)

    def test_the_real_list_names_the_porting_guide_sample(self) -> None:
        # The page whose whole job is to be copied by whoever ports the next board. It was outside
        # both guards' reach; the naming contract and the functional demo are now compiled, and each
        # of the two markers has to keep selecting a block (`findings()` reports a stale one).
        docs = [sample.doc for sample in guard.SAMPLES]
        self.assertIn("docs/hardware/porting.md", docs)
        sample = [s for s in guard.SAMPLES if s.doc == "docs/hardware/porting.md"][0]
        self.assertEqual(2, len(sample.markers), sample.markers)
        self.assertEqual([], [root for root in sample.roots if not (ROOT / root).is_dir()])


if __name__ == "__main__":
    unittest.main()
