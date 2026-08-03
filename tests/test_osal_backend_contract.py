#!/usr/bin/env python3
"""Regression tests for OSAL backend/public-header contract drift."""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OSAL = ROOT / "components" / "kernel" / "osal"
HEADER = OSAL / "xy_os.h"
BACKENDS = {
    "baremetal": OSAL / "backend" / "baremetal" / "xy_os_baremetal.c",
    "freertos": OSAL / "backend" / "freertos" / "xy_os_freertos.c",
    "rtthread": OSAL / "backend" / "rtthread" / "xy_os_rtthread.c",
    "cmsis_rtx": OSAL / "backend" / "cmsis_rtx" / "xy_os_cmsis_rtx.c",
}

DECL_RE = re.compile(
    r"^([A-Za-z_][\w\s\*]*?)\s+(xy_os_\w+)\s*\(([^;{}]*)\)\s*;",
    re.MULTILINE,
)
DEF_RE = re.compile(
    r"\n([A-Za-z_][\w\s\*]*?)\s+(xy_os_\w+)\s*\(([^;{}]*)\)\s*\{",
    re.DOTALL,
)
OBSOLETE_BACKEND_TOKENS = (
    "xy_os_error_t",
    "xy_os_callback_t",
    "XY_OS_ERROR_FAIL",
    "XY_OS_ERROR_INVALID_PARAM",
)


def _canonical_space(text: str) -> str:
    return " ".join(text.split())


def _collect_public_declarations(text: str):
    return {
        match.group(2): _canonical_space(match.group(1))
        for match in DECL_RE.finditer(text)
    }


def _collect_backend_definitions(text: str):
    return {
        match.group(2): _canonical_space(match.group(1))
        for match in DEF_RE.finditer(text)
    }


class OsalBackendContractTest(unittest.TestCase):
    def setUp(self):
        self.public = _collect_public_declarations(HEADER.read_text(encoding="utf-8"))
        self.assertGreater(len(self.public), 0)

    def test_all_backends_define_the_public_api_with_matching_return_types(self):
        for backend_name, backend_path in BACKENDS.items():
            with self.subTest(backend=backend_name):
                definitions = _collect_backend_definitions(backend_path.read_text(encoding="utf-8"))
                missing = sorted(set(self.public) - set(definitions))
                extra = sorted(set(definitions) - set(self.public))
                wrong_return = {
                    name: (self.public[name], definitions[name])
                    for name in sorted(set(self.public) & set(definitions))
                    if self.public[name] != definitions[name]
                }

                self.assertEqual([], missing)
                self.assertEqual([], extra)
                self.assertEqual({}, wrong_return)

    def test_backend_sources_do_not_use_removed_public_compat_names(self):
        for backend_name, backend_path in BACKENDS.items():
            text = backend_path.read_text(encoding="utf-8")
            with self.subTest(backend=backend_name):
                offenders = [token for token in OBSOLETE_BACKEND_TOKENS if token in text]
                self.assertEqual([], offenders)


if __name__ == "__main__":
    unittest.main()
