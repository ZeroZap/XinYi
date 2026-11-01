#!/usr/bin/env python3
"""Self-test for the dependency-closure rule.

The rule holds a document to a tree, so every case here is about one of the four ways they can disagree:
a subtree the document never heard of, a path it names that is gone, a licence claim the tree no longer
supports, and a summary that contradicts its own entries. Each is a fixture, because a rule that only
ever reads the real document is a rule nobody can see working.
"""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_dependency_closure as guard  # noqa: E402

#: Real licence wording, because the rule detects by the text a licence actually contains: a fixture
#: that writes "MIT License" and expects MIT detection is testing the fixture, not the rule.
MIT_TEXT = ("MIT License\n\nPermission is hereby granted, free of charge, to any person obtaining a "
            "copy of this software\n")
APACHE_TEXT = "Apache License\nVersion 2.0, January 2004\n\n"


class ClosureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "docs/validation").mkdir(parents=True)

    def subtree(self, relative: str, licence_text: str | None = None) -> None:
        path = self.root / relative
        path.mkdir(parents=True, exist_ok=True)
        (path / "source.c").write_text("int x;\n", encoding="utf-8")
        if licence_text is not None:
            (path / "LICENSE").write_text(licence_text, encoding="utf-8")

    def document(self, entries: list[dict], measurement: dict | None = None) -> None:
        payload = {
            "entries": entries,
            "measurement": measurement if measurement is not None
            else {"subtrees": len(entries),
                  "with_licence_evidence": sum(1 for e in entries if e.get("licence"))},
        }
        (self.root / guard.CLOSURE).write_text(json.dumps(payload), encoding="utf-8")

    def judge(self, pins: dict[str, str] | None = None) -> list[str]:
        return guard.findings(self.root, {} if pins is None else pins)

    def test_a_vendored_subtree_the_document_never_heard_of_is_reported(self):
        self.subtree("components/third_party/usb/tinyusb", MIT_TEXT)
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"}])
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("tinyusb", problems[0])
        self.assertIn("not in the dependency closure", problems[0])

    def test_a_listed_path_that_is_gone_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        # The summary is set to what the tree really evidences (one subtree, one licence) so the only
        # finding is the missing directory, not a count that the absent entry also disturbed.
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"},
                       {"path": "components/dm/vanished", "licence": "MIT"}],
                      measurement={"subtrees": 2, "with_licence_evidence": 1})
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("cannot reproduce it", problems[0])

    def test_a_stale_licence_claim_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([{"path": "components/dm/coreJSON", "licence": "Apache-2.0"}])
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("the claim is stale", problems[0])

    def test_an_unpinned_subtree_with_no_licence_is_reported(self):
        self.subtree("components/dm/coreJSON")
        self.document([{"path": "components/dm/coreJSON", "licence": None}])
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("no licence evidence and no pin", problems[0])

    def test_a_pinned_subtree_with_no_licence_passes(self):
        self.subtree("components/dm/coreJSON")
        self.document([{"path": "components/dm/coreJSON", "licence": None}])
        self.assertEqual([], self.judge({"components/dm/coreJSON": "upstream unknown"}))

    def test_a_pin_without_a_reason_is_reported(self):
        self.subtree("components/dm/coreJSON")
        self.document([{"path": "components/dm/coreJSON", "licence": None}])
        problems = self.judge({"components/dm/coreJSON": "  "})
        self.assertEqual(1, len(problems))
        self.assertIn("what is missing", problems[0])

    def test_a_pin_whose_licence_appeared_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"}])
        problems = self.judge({"components/dm/coreJSON": "upstream unknown"})
        self.assertEqual(1, len(problems))
        self.assertIn("the pin has expired", problems[0])

    def test_a_pin_for_an_unlisted_path_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"}])
        problems = self.judge({"components/dm/ghost": "upstream unknown"})
        self.assertEqual(1, len(problems))
        self.assertIn("does not list it", problems[0])

    def test_a_summary_that_contradicts_the_entries_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"}],
                      measurement={"subtrees": 1, "with_licence_evidence": 0})
        problems = self.judge()
        self.assertTrue(any("contradicts itself" in p for p in problems), problems)

    def test_a_pinned_submodule_absent_from_the_tree_is_accepted(self):
        # A clean checkout has the gitlink and not the content: the licence cannot be re-read, so the
        # record is checked instead of claiming a verification that did not happen.
        self.document([{"path": "components/third_party/flashdb", "licence": "Apache-2.0",
                        "submodule": {"url": "https://example.invalid/flashdb.git",
                                      "revision": "0123456789abcdef"}}])
        self.assertEqual([], self.judge())

    def test_an_absent_path_recorded_as_a_submodule_without_a_revision_is_reported(self):
        self.document([{"path": "components/third_party/flashdb", "licence": "Apache-2.0",
                        "submodule": {"url": "https://example.invalid/flashdb.git"}}],
                      measurement={"subtrees": 1, "with_licence_evidence": 0})
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("cannot reproduce it", problems[0])

    def test_an_absent_path_not_recorded_as_a_submodule_is_reported(self):
        self.document([{"path": "components/dm/gone", "licence": "MIT"}],
                      measurement={"subtrees": 1, "with_licence_evidence": 0})
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("cannot reproduce it", problems[0])

    def test_a_licence_declared_only_by_an_spdx_tag_is_detected(self):
        # Several subtrees in this tree carry no LICENSE file and declare the licence per source file.
        # Without the SPDX path they read as unlicensed, which would turn a closed gap into a false one.
        path = self.root / "components/dm/coreJSON"
        path.mkdir(parents=True)
        (path / "core_json.c").write_text("/* SPDX-License-Identifier: MIT */\nint x;\n",
                                          encoding="utf-8")
        self.document([{"path": "components/dm/coreJSON", "licence": "MIT"}])
        self.assertEqual([], self.judge())

    def test_a_subtree_with_neither_a_licence_file_nor_a_tag_has_no_evidence(self):
        self.subtree("components/dm/coreJSON")
        self.document([{"path": "components/dm/coreJSON", "licence": None}])
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("no licence evidence and no pin", problems[0])

    def test_a_missing_document_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("has not been measured", problems[0])

    def test_an_invalid_document_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        (self.root / guard.CLOSURE).write_text("{not json", encoding="utf-8")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("not valid JSON", problems[0])

    def test_a_document_with_no_entries_is_reported(self):
        self.subtree("components/dm/coreJSON", MIT_TEXT)
        self.document([])
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("nothing has been measured", problems[0])

    def test_the_real_document_matches_the_real_tree(self):
        self.assertEqual([], guard.findings(guard.ROOT))


class DiscoveryTests(unittest.TestCase):
    def test_an_aggregate_directory_is_not_itself_a_dependency(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "components/third_party/usb/tinyusb").mkdir(parents=True)
            discovered = guard.discover(root)
            self.assertNotIn("components/third_party", discovered, "an aggregate is not a dependency")
            self.assertIn("components/third_party/usb/tinyusb", discovered)


if __name__ == "__main__":
    unittest.main()
