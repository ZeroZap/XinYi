#!/usr/bin/env python3
"""Tests for the AT modem simulator, in-process and over a pseudo-terminal.

Two layers on purpose. The engine tests answer "does it speak the protocol"; the pty tests answer "can a
separate client actually reach it", which is the only thing that makes it useful for testing an AT client
written in C rather than in the same process.

The pty cases read both directions explicitly. Reading only the master side returns the bytes the *client*
sent, which looks like a plausible response until you notice the modem's answer is not in it.
"""

from __future__ import annotations

import os
import select
import sys
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from at_modem_sim import PROFILES, AtModem, ModemConfig, PtyTransport  # noqa: E402


class EngineTests(unittest.TestCase):
    def modem(self, profile: str = "ec2x", **config) -> tuple[AtModem, list[bytes]]:
        written: list[bytes] = []
        return AtModem(written.append, PROFILES[profile], ModemConfig(**config)), written

    def ask(self, modem: AtModem, written: list[bytes], command: str) -> str:
        written.clear()
        modem.feed((command + "\r\n").encode())
        return b"".join(written).decode()

    def test_a_plain_command_is_answered_ok(self):
        modem, written = self.modem()
        self.assertEqual("\r\nOK\r\n", self.ask(modem, written, "AT"))

    def test_an_information_command_returns_its_lines_then_ok(self):
        modem, written = self.modem()
        self.assertEqual("\r\n+CSQ: 18,0\r\n\r\nOK\r\n", self.ask(modem, written, "AT+CSQ"))

    def test_an_unknown_command_is_answered_error(self):
        modem, written = self.modem()
        self.assertEqual("\r\nERROR\r\n", self.ask(modem, written, "AT+NONSENSE"))

    def test_the_command_is_not_echoed_by_default(self):
        modem, written = self.modem()
        self.assertNotIn("AT+CSQ", self.ask(modem, written, "AT+CSQ"))

    def test_echo_is_honoured_when_enabled(self):
        modem, written = self.modem(echo=True)
        self.assertTrue(self.ask(modem, written, "AT").startswith("AT\r\n"))

    def test_an_upload_command_prompts_then_acknowledges_the_declared_length(self):
        modem, written = self.modem()
        self.assertEqual("\r\n> ", self.ask(modem, written, "AT+QISEND=0,5"))
        written.clear()
        modem.feed(b"abcde")
        self.assertEqual("\r\nSEND OK\r\n", b"".join(written).decode())
        self.assertEqual([b"abcde"], modem.uploads)

    def test_an_upload_that_is_one_byte_short_is_not_acknowledged(self):
        modem, written = self.modem()
        self.ask(modem, written, "AT+QISEND=0,5")
        written.clear()
        modem.feed(b"abcd")
        self.assertEqual("", b"".join(written).decode())
        self.assertEqual(1, modem._upload_remaining)

    def test_the_sim76_upload_keyword_opens_the_same_framing(self):
        modem, written = self.modem("sim76")
        self.assertEqual("\r\n> ", self.ask(modem, written, "AT+CIPSEND=0,3"))
        written.clear()
        modem.feed(b"xyz")
        self.assertIn("SEND OK", b"".join(written).decode())

    def test_the_esp_upload_keyword_takes_a_bare_length(self):
        modem, written = self.modem("esp_at")
        self.assertEqual("\r\n> ", self.ask(modem, written, "AT+CIPSEND=4"))
        written.clear()
        modem.feed(b"wxyz")
        self.assertIn("SEND OK", b"".join(written).decode())

    def test_a_dropped_command_gets_no_answer_at_all(self):
        # The fault that catches a client claiming success: accepted on the wire, never answered.
        modem, written = self.modem(drop_commands=[__import__("re").compile(r"AT\+DROP")])
        self.assertEqual("", self.ask(modem, written, "AT+DROP"))
        self.assertEqual("\r\nOK\r\n", self.ask(modem, written, "AT"))

    def test_a_forced_error_is_answered_error(self):
        modem, written = self.modem(error_commands=[__import__("re").compile(r"AT\+CSQ")])
        self.assertEqual("\r\nERROR\r\n", self.ask(modem, written, "AT+CSQ"))

    def test_an_injected_urc_is_written_as_its_own_frame(self):
        modem, written = self.modem()
        written.clear()
        modem.inject("+QIURC: \"recv\",0,2")
        self.assertEqual("\r\n+QIURC: \"recv\",0,2\r\n", b"".join(written).decode())

    def test_a_fragmented_answer_arrives_in_pieces_that_join_back_up(self):
        modem, written = self.modem(fragment_size=3)
        self.assertEqual("\r\nOK\r\n", self.ask(modem, written, "AT"))
        self.assertGreater(len(written), 1, "the answer must be split into more than one write")

    def test_a_bare_line_feed_also_terminates_a_command(self):
        modem, written = self.modem()
        written.clear()
        modem.feed(b"AT\n")
        self.assertIn("OK", b"".join(written).decode())

    def test_a_command_split_across_two_chunks_is_reassembled(self):
        modem, written = self.modem()
        written.clear()
        modem.feed(b"AT+CS")
        self.assertEqual("", b"".join(written).decode())
        modem.feed(b"Q\r\n")
        self.assertIn("+CSQ", b"".join(written).decode())


class PtyHarness:
    """A modem on one end of a pseudo-terminal and a client fd on the other."""

    def __init__(self, profile: str = "ec2x", **config) -> None:
        self.transport = PtyTransport()
        self.modem = AtModem(self.transport.write, PROFILES[profile], ModemConfig(**config))
        self.client = os.open(self.transport.path, os.O_RDWR | os.O_NOCTTY)
        os.set_blocking(self.client, False)

    def pump(self, settle_ms: int = 60) -> bytes:
        received = b""
        deadline = time.monotonic() + settle_ms / 1000.0
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.transport.master], [], [], 0.005)
            if ready:
                data = self.transport.read()
                if data:
                    self.modem.feed(data)
            ready, _, _ = select.select([self.client], [], [], 0.005)
            if ready:
                try:
                    received += os.read(self.client, 4096)
                except (BlockingIOError, OSError):
                    pass
        return received

    def send(self, data: bytes, settle_ms: int = 60) -> bytes:
        os.write(self.client, data)
        return self.pump(settle_ms)

    def close(self) -> None:
        try:
            os.close(self.client)
        except OSError:
            pass
        self.transport.close()


class PtyTests(unittest.TestCase):
    def test_a_command_round_trips_over_a_real_pty(self):
        harness = PtyHarness()
        self.addCleanup(harness.close)
        self.assertIn(b"OK", harness.send(b"AT\r\n"))

    def test_the_client_does_not_receive_its_own_bytes(self):
        harness = PtyHarness()
        self.addCleanup(harness.close)
        self.assertNotIn(b"AT+CSQ", harness.send(b"AT+CSQ\r\n"))

    def test_an_upload_round_trips_over_a_pty(self):
        harness = PtyHarness()
        self.addCleanup(harness.close)
        self.assertIn(b">", harness.send(b"AT+QISEND=0,3\r\n"))
        self.assertIn(b"SEND OK", harness.send(b"abc", settle_ms=120))
        self.assertEqual([b"abc"], harness.modem.uploads)

    def test_an_injected_urc_reaches_the_client(self):
        harness = PtyHarness()
        self.addCleanup(harness.close)
        harness.modem.inject("+QIURC: \"recv\",0,1")
        self.assertIn(b"+QIURC", harness.pump(80))


if __name__ == "__main__":
    unittest.main()
