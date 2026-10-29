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

    def test_an_offered_backend_without_a_branch_is_reported(self) -> None:
        """Selectable and offered, but the build compiles no backend for it."""
        osal = OSAL_OK.replace('elseif(OSAL_BACKEND STREQUAL "freertos")\n', "")
        problems = self.run_guard(ROOT_OK, osal, UNREACHABLE)
        self.assertEqual(len(problems), 1)
        self.assertIn("build no backend", problems[0])
        self.assertIn("freertos", problems[0])

    def test_a_commented_out_branch_is_not_a_branch(self) -> None:
        """The same deletion written as a comment must be caught too - prose is not a branch."""
        osal = OSAL_OK.replace('elseif(OSAL_BACKEND STREQUAL "freertos")\n',
                               '# elseif(OSAL_BACKEND STREQUAL "freertos")\n')
        self.assertNotIn("freertos", guard.dispatched(osal))
        problems = self.run_guard(ROOT_OK, osal, UNREACHABLE)
        self.assertEqual(len(problems), 1)
        self.assertIn("freertos", problems[0])

    def test_a_commented_out_advertisement_is_not_an_advertisement(self) -> None:
        osal = ("# set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos)\n"
                'if(OSAL_BACKEND STREQUAL "baremetal")\n'
                'elseif(OSAL_BACKEND STREQUAL "freertos")\nendif()\n')
        self.assertEqual(set(), guard.advertised(osal))
        # Nothing is advertised at all, and the guard refuses to call that agreement.
        self.assertEqual(1, len(self.run_guard(ROOT_OK, osal, {})))

    def test_a_commented_out_root_selection_is_not_selectable(self) -> None:
        root = ROOT_OK.replace(
            'set(OSAL_BACKEND "freertos" CACHE STRING "OSAL backend selection" FORCE)',
            '# set(OSAL_BACKEND "freertos" CACHE STRING "OSAL backend selection" FORCE)')
        self.assertEqual({"baremetal"}, guard.selectable(root))
        problems = self.run_guard(root, OSAL_OK, UNREACHABLE)
        self.assertEqual(len(problems), 1)
        self.assertIn("can only select", problems[0])

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


class DirectoryTests(unittest.TestCase):
    """A backend directory that nothing dispatches to is an implementation nobody can build."""

    def test_a_dispatched_directory_passes(self):
        self.assertEqual([], guard.directory_findings({"freertos"}, {}, {}, {"freertos"}))

    def test_an_undeclared_directory_is_reported(self):
        problems = guard.directory_findings({"freertos"}, {}, {}, {"freertos", "rtx5"})
        self.assertEqual(1, len(problems))
        self.assertIn("say which it is", problems[0])

    def test_an_orphaned_declaration_covers_it(self):
        self.assertEqual([],
                         guard.directory_findings({"freertos"}, {}, {"rtx5": "why"}, {"freertos", "rtx5"}))

    def test_a_declaration_for_a_missing_directory_is_reported(self):
        problems = guard.directory_findings(set(), {"gone": "why"}, {}, set())
        self.assertEqual(1, len(problems))
        self.assertIn("attached to nothing", problems[0])

    def test_the_real_tree_classifies_every_backend(self):
        branches = guard.dispatched(guard.OSAL_CMAKE.read_text(encoding="utf-8"))
        on_disk = guard.backend_directories(guard.OSAL_CMAKE.parent)
        self.assertTrue(on_disk, "the walk found no backend directories")
        self.assertEqual([], guard.directory_findings(branches, guard.UNREACHABLE,
                                                     guard.ORPHANED, on_disk))
        self.assertIn("rtx5", guard.ORPHANED)


class UndefinedSymbolTests(unittest.TestCase):
    """A condition on a symbol the parsed Kconfig never defines guards a branch nobody can take."""

    CMAKE = 'if(CONFIG_OSAL_BACKEND_FREERTOS)\n    list(APPEND OSAL_SOURCES backend_source)\nendif()\n'
    KCONFIG = 'config OSAL_BACKEND_FREERTOS\n    bool "FreeRTOS"\n'

    def test_a_defined_symbol_passes(self):
        self.assertEqual([], guard.undefined_symbol_findings(self.CMAKE, self.KCONFIG, {}))

    def test_an_undefined_symbol_is_reported(self):
        problems = guard.undefined_symbol_findings(self.CMAKE, "", {})
        self.assertEqual(1, len(problems))
        self.assertIn("can never be taken", problems[0])

    def test_a_pinned_symbol_passes(self):
        self.assertEqual([], guard.undefined_symbol_findings(
            self.CMAKE, "", {"CONFIG_OSAL_BACKEND_FREERTOS": "declared in a file nothing parses"}))

    def test_a_pin_that_closes_is_reported(self):
        """Defining the symbol makes the pin stale - the gap closed and the record has to say so."""
        problems = guard.undefined_symbol_findings(
            self.CMAKE, self.KCONFIG, {"CONFIG_OSAL_BACKEND_FREERTOS": "old reason"})
        self.assertEqual(1, len(problems))
        self.assertIn("the pin is stale", problems[0])

    def test_a_parser_that_finds_no_condition_fails(self):
        problems = guard.undefined_symbol_findings("message(STATUS \"hi\")\n", self.KCONFIG, {})
        self.assertEqual(1, len(problems))
        self.assertIn("a parser that finds nothing", problems[0])

    def test_symbols_outside_conditions_are_not_judged(self) -> None:
        """A CONFIG_ token in a message or a variable name is not a branch."""
        text = 'message(STATUS "backend: ${CONFIG_OSAL_BACKEND}")\nset(OSAL_CONFIG_DIR x)\n'
        self.assertEqual(set(), guard.condition_symbols(text))

    def test_a_trailing_comment_on_a_condition_is_not_a_symbol(self) -> None:
        """`if(CONFIG_A) # CONFIG_B` branches on one symbol; the second is prose."""
        text = "if(CONFIG_OSAL_BACKEND_FREERTOS) # CONFIG_OSAL_BACKEND_RTX5 would go here\n"
        self.assertEqual({"CONFIG_OSAL_BACKEND_FREERTOS"}, guard.condition_symbols(text))

    def test_a_commented_out_kconfig_declaration_is_not_a_definition(self) -> None:
        text = "# config OSAL_BACKEND_RTX5\nconfig OSAL_BACKEND_FREERTOS\n"
        self.assertEqual({"CONFIG_OSAL_BACKEND_FREERTOS"}, guard.parsed_symbols(text))


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

    def test_a_commented_out_fenced_command_is_not_an_instruction(self):
        """Writing down that a spelling is rejected is not telling readers to use it."""
        docs = {"docs/design/x.md":
                "```bash\n# cmake .. -DOSAL_BACKEND=rtthread   <- refused, see above\n"
                "cmake .. -DOSAL_BACKEND=freertos\n```\n"}
        self.assertEqual(guard.doc_command_findings(docs, {"baremetal", "freertos"}), [])

    def test_a_backend_named_inside_a_command_line_is_still_read(self):
        """Stripping the comment must not delete quoted text - that is data, not prose."""
        docs = {"docs/design/x.md":
                '```bash\nsh -c "cmake .. -DOSAL_BACKEND=rtthread"   # the refused spelling\n```\n'}
        problems = guard.doc_command_findings(docs, {"baremetal", "freertos"})
        self.assertEqual(len(problems), 1)
        self.assertIn("-DOSAL_BACKEND=rtthread", problems[0])

    def test_a_tilde_fence_is_a_fence(self):
        docs = {"docs/design/x.md": "~~~bash\ncmake .. -DOSAL_BACKEND=rtthread\n~~~\n"}
        problems = guard.doc_command_findings(docs, {"baremetal", "freertos"})
        self.assertEqual(len(problems), 1)
        self.assertIn("-DOSAL_BACKEND=rtthread", problems[0])

    def test_an_unterminated_fence_still_shows_its_command(self):
        docs = {"docs/design/x.md": "```bash\ncmake .. -DOSAL_BACKEND=rtthread\n"}
        problems = guard.doc_command_findings(docs, {"baremetal", "freertos"})
        self.assertEqual(len(problems), 1)

    def test_two_fences_are_two_blocks(self):
        docs = {"docs/design/x.md":
                "```bash\ncmake .. -DOSAL_BACKEND=freertos\n```\n\ntext\n\n"
                "```bash\ncmake .. -DOSAL_BACKEND=rtthread\n```\n"}
        problems = guard.doc_command_findings(docs, {"baremetal", "freertos"})
        self.assertEqual(len(problems), 1)
        self.assertIn("-DOSAL_BACKEND=rtthread", problems[0])


class PlatformCommandTests(unittest.TestCase):
    """A fenced command must name the platform variable this build reads, with a value it accepts."""

    def test_a_command_passing_platform_without_hal_is_reported(self):
        docs = {"docs/getting-started/x.md": "```bash\nmake PLATFORM=PC\n```\n"}
        problems = guard.platform_command_findings(docs)
        self.assertEqual(len(problems), 1)
        self.assertIn("PLATFORM=PC", problems[0])
        self.assertIn("HAL_PLATFORM", problems[0])

    def test_a_command_passing_the_variable_the_build_reads_passes(self):
        docs = {"docs/getting-started/x.md": "```bash\nmake HAL_PLATFORM=STM32U5\n```\n"}
        self.assertEqual(guard.platform_command_findings(docs), [])

    def test_a_cmake_d_flag_is_judged_the_same_way(self):
        docs = {"docs/getting-started/x.md": "```bash\ncmake .. -DPLATFORM=PC\n```\n"}
        problems = guard.platform_command_findings(docs)
        self.assertEqual(len(problems), 1)
        self.assertIn("PLATFORM=PC", problems[0])

    def test_a_makefile_fragment_is_not_a_command(self):
        """A fragment showing how another project spells its own build is not something a reader pastes."""
        docs = {"docs/hardware/x.md": "```makefile\nPLATFORM = stm32\n```\n"}
        self.assertEqual(guard.platform_command_findings(docs), [])

    def test_a_snippet_naming_a_value_the_build_refuses_is_reported(self):
        """Not a command, and still wrong: a reader would write this into their own build."""
        docs = {"docs/getting-started/x.md": "```makefile\nHAL_PLATFORM = STM32\n```\n"}
        problems = guard.platform_command_findings(docs)
        self.assertEqual(len(problems), 1)
        self.assertIn("HAL_PLATFORM=STM32", problems[0])
        self.assertIn("STM32U5", problems[0])

    def test_the_real_reader_docs_pass(self):
        docs = guard.reader_docs()
        self.assertTrue(docs)
        self.assertEqual(guard.platform_command_findings(docs), [])


class MakeTargetTests(unittest.TestCase):
    """A fenced `make <target>` must name a target the root Makefile defines."""

    MAKEFILE = """
.PHONY: all test-unit
all: build
test-unit:
\t@echo test

$(BUILD_DIR)/CMakeCache.txt:
\t@cmake -B $(BUILD_DIR)

%.tmp: %.in
\t@echo $<
"""

    def test_a_target_the_makefile_defines_passes(self):
        docs = {"README.md": "```bash\nmake test-unit\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_a_target_the_makefile_lacks_is_reported(self):
        docs = {"README.md": "```bash\nmake menuconfig\n```\n"}
        problems = guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE))
        self.assertEqual(len(problems), 1)
        self.assertIn("make menuconfig", problems[0])

    def test_a_prompt_prefixed_command_is_judged_too(self):
        docs = {"README.md": "```sh\n$ make flash\n```\n"}
        problems = guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE))
        self.assertEqual(len(problems), 1)
        self.assertIn("make flash", problems[0])

    def test_a_commented_out_command_is_not_an_instruction(self):
        """The same rule as everywhere else: a `#` in a fence is a comment, not advice."""
        docs = {"README.md": "```bash\n# make flash   <- there is no such target\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_a_block_that_changed_directory_is_not_the_root_makefile(self):
        """CMake's generated Makefile does know the library targets; the root one does not."""
        docs = {"README.md": "```bash\nmkdir build && cd build\ncmake ..\nmake xy_crypto\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_make_dash_c_names_another_makefile(self):
        docs = {"README.md": "```bash\nmake -C tests/qemu_stm32f4 hal_test\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_variable_assignments_and_options_are_not_targets(self):
        docs = {"README.md": "```bash\nmake -j8 HAL_PLATFORM=STM32U5 BUILD_TYPE=debug\n"
                             "make --version\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_a_redirection_ends_the_argument_list(self):
        docs = {"README.md": "```bash\nmake test-unit 2>&1 | tee log\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_a_path_that_starts_with_make_is_not_a_command(self):
        docs = {"README.md": "```text\nmake/\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_a_proposal_document_is_not_an_instruction(self):
        """A document arguing for a target that does not exist yet is its subject, not its defect."""
        where = sorted(guard.PROPOSAL_DOCUMENTS)[0]
        docs = {where: "```bash\nmake test-integration\n```\n"}
        self.assertEqual(guard.make_target_findings(docs, guard.makefile_targets(self.MAKEFILE)),
                         [])

    def test_the_root_makefile_parser_finds_its_targets(self):
        targets = guard.makefile_targets(guard.read_hash_code(guard.ROOT_MAKEFILE))
        self.assertIn("test-unit", targets)
        self.assertIn("test-tools", targets)
        self.assertNotIn("menuconfig", targets)

    def test_the_real_reader_docs_pass(self):
        docs = guard.reader_docs()
        self.assertTrue(docs)
        targets = guard.makefile_targets(guard.read_hash_code(guard.ROOT_MAKEFILE))
        self.assertEqual(guard.make_target_findings(docs, targets), [])


class FencedBlockTests(unittest.TestCase):
    def test_a_four_backtick_fence_is_not_closed_by_three(self):
        """A block that quotes a fenced example keeps its content instead of ending inside it."""
        text = "````markdown\n```bash\ncmake .. -DOSAL_BACKEND=rtthread\n```\n````\n"
        blocks = guard.fenced_blocks(text)
        self.assertEqual(len(blocks), 1)
        self.assertIn("-DOSAL_BACKEND=rtthread", blocks[0])

    def test_a_four_space_indented_line_is_not_a_fence(self):
        self.assertEqual(guard.fenced_blocks("    ```bash\ncmake ..\n    ```\n"), [])

    def test_the_real_reader_docs_are_read_as_blocks(self):
        """A parser that finds no fence is green for the same reason an empty file is."""
        docs = guard.reader_docs()
        self.assertTrue(docs)
        self.assertTrue(any(guard.fenced_blocks(text) for text in docs.values()))


class RepositoryTests(unittest.TestCase):
    def test_the_repository_passes(self):
        root = guard.ROOT_CMAKE.read_text(encoding="utf-8")
        problems = guard.findings(root, guard.OSAL_CMAKE.read_text(encoding="utf-8"))
        docs = guard.reader_docs()
        problems += guard.doc_command_findings(docs, guard.selectable(root))
        problems += guard.platform_command_findings(docs)
        problems += guard.make_target_findings(
            docs, guard.makefile_targets(guard.read_hash_code(guard.ROOT_MAKEFILE)))
        self.assertEqual(problems, [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
