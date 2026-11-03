#!/usr/bin/env python3
"""Cases for the single-comment-stripper guard.

The guard exists because seven guards each carried a stripper and four of them used the
regular-expression version, which treats a comment marker inside a string literal as a
comment. These cases pin that it recognises both forms of duplication - the inline regex and
the local redefinition - and that it leaves the shared implementation alone.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_single_comment_stripper as guard  # noqa: E402


class StripperTests(unittest.TestCase):
    def scan(self, text: str, name: str = "check_example.py") -> list[str]:
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "tests" / "unit").mkdir(parents=True)
            (root / "tests" / "unit" / name).write_text(text, encoding="utf-8")
            return guard.findings(root, root / "tests" / "unit" / "support")

    def test_an_inline_block_comment_regex_is_reported(self):
        source = 'import re\ntext = re.sub(r"/\\*.*?\\*/", " ", text, flags=re.S)\n'
        problems = self.scan(source)
        self.assertEqual(len(problems), 1)
        self.assertIn("source_scan.code_only", problems[0])

    def test_an_inline_line_comment_regex_is_reported(self):
        source = 'import re\ntext = re.sub(r"//[^\\n]*", "", text)\n'
        self.assertEqual(len(self.scan(source)), 1)

    def test_a_local_definition_without_the_shared_import_is_reported(self):
        source = "def strip_comments(text: str) -> str:\n    return text\n"
        problems = self.scan(source)
        self.assertEqual(len(problems), 1)
        self.assertIn("own comment stripper", problems[0])

    def test_a_local_definition_after_the_shared_import_is_allowed(self):
        # Guards keep a thin wrapper name; what matters is that the body delegates.
        source = ('from source_scan import code_only\n\n\n'
                  'def strip_comments(text: str) -> str:\n    return code_only(text)\n')
        self.assertEqual(self.scan(source), [])

    def test_a_local_hash_stripper_is_reported(self):
        """The `#` half is the same rule: one implementation, shared from support/."""
        source = "def strip_hash_comments(text: str) -> str:\n    return text\n"
        problems = self.scan(source)
        self.assertEqual(len(problems), 1)
        self.assertIn("own comment stripper", problems[0])

    def test_a_guard_that_does_not_strip_comments_is_not_blamed(self):
        self.assertEqual(self.scan("import re\nname = re.sub(r'a', 'b', name)\n"), [])

    def test_the_shared_implementation_is_never_reported(self):
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            support = root / "tests" / "unit" / "support"
            support.mkdir(parents=True)
            (support / "source_scan.py").write_text(
                'import re\ndef code_only(text):\n    return re.sub(r"/\\*.*?\\*/", "", text)\n',
                encoding="utf-8")
            self.assertEqual(guard.findings(root, support), [])


class RepositoryTests(unittest.TestCase):
    def test_the_repository_has_exactly_one_stripper(self):
        self.assertEqual(guard.findings(), [])

    def test_the_shared_implementation_is_where_the_guard_expects_it(self):
        self.assertTrue((guard.SUPPORT / "source_scan.py").is_file())


if __name__ == "__main__":
    unittest.main(verbosity=1)
