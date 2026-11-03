#!/usr/bin/env python3
"""Cases for the PC coverage guard.

The guard's job is to keep a number honest, so the cases drive it with synthetic header/source trees
and check that both directions of drift are reported: an entry that disappears, and an entry that
gets implemented without the pin being tightened.
"""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_pc_coverage as guard  # noqa: E402

HEADER = """
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);
xy_hal_error_t xy_hal_uart_send(void *uart, const void *data, size_t len);
int xy_hal_uart_recv(void *uart, void *data, size_t len, uint32_t timeout);
"""

PC_SOURCE = """
xy_hal_error_t xy_hal_uart_send(void *uart, const void *data, size_t len)
{
    return XY_HAL_OK;
}

int xy_hal_uart_recv(void *uart, void *data, size_t len, uint32_t timeout)
{
    return 0;
}

int xy_hal_uart_recv_pc(void *uart, void *data, size_t len)
{
    return 0;
}
"""


class Tree:
    """A throwaway headers-plus-PC-backend pair."""

    def __init__(self, header=HEADER, source=PC_SOURCE):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.inc = root / "inc"
        self.pc = root / "PC"
        self.inc.mkdir()
        self.pc.mkdir()
        (self.inc / "xy_hal_uart.h").write_text(header)
        (self.pc / "xy_hal_uart_pc.c").write_text(source)

    def findings(self, pinned):
        return guard.findings(self.inc, self.pc, pinned)

    def close(self):
        self.tmp.cleanup()


class PinnedTests(unittest.TestCase):
    def setUp(self):
        self.trees = []

    def tearDown(self):
        for tree in self.trees:
            tree.close()

    def tree(self, **kwargs):
        made = Tree(**kwargs)
        self.trees.append(made)
        return made

    def test_the_pin_matching_reality_is_quiet(self):
        tree = self.tree()
        self.assertEqual(tree.findings({"uart": {"xy_hal_uart_init"}}), [])

    def test_the_internal_pc_helper_is_not_counted_as_the_public_entry(self):
        # xy_hal_uart_recv_pc exists; that says nothing about xy_hal_uart_recv.
        tree = self.tree(source=PC_SOURCE.replace(
            "int xy_hal_uart_recv(", "int xy_hal_uart_recv_unused(", 1))
        problems = tree.findings({"uart": {"xy_hal_uart_init"}})
        self.assertTrue(any("xy_hal_uart_recv" in p and "newly missing" in p for p in problems))

    def test_a_newly_missing_entry_is_reported(self):
        tree = self.tree()
        problems = tree.findings({"uart": set()})
        self.assertTrue(any("newly missing" in p and "xy_hal_uart_init" in p for p in problems))

    def test_implementing_an_entry_without_tightening_the_pin_is_reported(self):
        tree = self.tree()
        problems = tree.findings({"uart": {"xy_hal_uart_init", "xy_hal_uart_send"}})
        self.assertTrue(any("has been implemented" in p and "xy_hal_uart_send" in p
                            for p in problems))

    def test_a_pinned_family_with_no_header_is_reported(self):
        tree = self.tree()
        problems = tree.findings({"uart": set(), "adc": {"xy_hal_adc_init"}})
        self.assertTrue(any("no header declares" in p for p in problems))

    def test_any_return_type_counts_as_a_definition(self):
        # xy_hal_uart_recv returns int; an earlier rule only listed a few types and misread it as
        # missing, which reported a real entry as absent.
        tree = self.tree()
        self.assertEqual(tree.findings({"uart": {"xy_hal_uart_init"}}), [])


class RepositoryTests(unittest.TestCase):
    def test_the_repository_matches_its_pin(self):
        self.assertEqual(guard.findings(), [])

    def test_the_inventory_is_not_empty(self):
        # A guard that measures nothing reports nothing, which looks exactly like success.
        actual = guard.measured()
        self.assertTrue(actual)
        self.assertGreater(sum(len(entries) for entries in actual.values()), 100)


if __name__ == "__main__":
    unittest.main(verbosity=1)
