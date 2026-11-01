#!/usr/bin/env python3
"""The AT simulator, driven over a real pseudo-terminal rather than called in-process.

The engine tests in `test_at_server_sim.py` prove the protocol logic; these prove it is reachable over a
byte stream by a separate client, which is the only thing that makes it useful for exercising an AT
client written in C. No pyserial and no hardware: a pseudo-terminal is in the standard library.

Each case reads both directions explicitly. Reading only the master side returns the bytes the *client*
sent - which looks like a plausible response until you notice the simulator's reply never appears in it.
"""

from __future__ import annotations

import os
import select
import sys
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from at_server_sim import ATServerSimulator, PtyTransport, SimulatorConfig  # noqa: E402


class PtyHarness:
    """A simulator on one end of a pseudo-terminal and a client fd on the other."""

    def __init__(self, config: SimulatorConfig | None = None) -> None:
        self.transport = PtyTransport()
        self.simulator = ATServerSimulator(self.transport.write, config or SimulatorConfig())
        self.client = os.open(self.transport.path, os.O_RDWR | os.O_NOCTTY)
        os.set_blocking(self.client, False)

    def pump(self, settle_ms: int = 60) -> bytes:
        """Move bytes both ways for `settle_ms` and return what the client received."""
        received = b""
        deadline = time.monotonic() + settle_ms / 1000.0
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.transport.master], [], [], 0.005)
            if ready:
                data = self.transport.read(4096)
                if data:
                    self.simulator.feed(data)
            ready, _, _ = select.select([self.client], [], [], 0.005)
            if ready:
                try:
                    received += os.read(self.client, 4096)
                except (BlockingIOError, OSError):
                    pass
        return received

    def send(self, command: bytes, settle_ms: int = 60) -> bytes:
        os.write(self.client, command)
        return self.pump(settle_ms)

    def close(self) -> None:
        try:
            os.close(self.client)
        except OSError:
            pass
        self.transport.close()


class PtyExchangeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.harness = PtyHarness()
        self.addCleanup(self.harness.close)

    def test_a_plain_command_gets_ok(self):
        self.assertIn(b"OK", self.harness.send(b"AT\r\n"))

    def test_a_query_gets_its_information_response(self):
        self.assertIn(b"+CSQ:", self.harness.send(b"AT+CSQ\r\n"))

    def test_an_unknown_command_gets_error(self):
        self.assertIn(b"ERROR", self.harness.send(b"AT+BOGUS\r\n"))

    def test_an_injected_urc_reaches_the_client(self):
        self.harness.simulator.inject(b"\r\n+QIURC: \"recv\",0,3\r\nhello\r\n")
        self.assertIn(b"+QIURC", self.harness.pump(80))

    def test_the_client_does_not_receive_its_own_bytes_echoed(self):
        # Raw mode on both ends is what makes this true; with echo on, a client would see every command
        # twice and a parser reading the stream would treat its own request as a response.
        self.assertNotIn(b"AT+CSQ", self.harness.send(b"AT+CSQ\r\n"))

    def test_a_fragmented_response_reassembles_at_the_client(self):
        self.harness.close()
        self.harness = PtyHarness(SimulatorConfig(fragment_size=3, fragment_delay_ms=0))
        self.addCleanup(self.harness.close)
        self.assertIn(b"+CSQ:", self.harness.send(b"AT+CSQ\r\n", settle_ms=120))


class PtyDataModeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.harness = PtyHarness(SimulatorConfig(profile="ec2x"))
        self.addCleanup(self.harness.close)

    def test_a_send_command_prompts_then_accepts_the_declared_length(self):
        self.assertIn(b">", self.harness.send(b"AT+QISEND=0,5\r\n"))
        self.assertIn(b"SEND OK", self.harness.send(b"abcde", settle_ms=120))
        self.assertEqual([b"abcde"], self.harness.simulator.payloads)

    def test_a_short_payload_stays_pending_rather_than_being_acknowledged(self):
        self.harness.send(b"AT+QISEND=0,10\r\n")
        response = self.harness.send(b"abcde", settle_ms=120)
        self.assertNotIn(b"SEND OK", response)
        self.assertEqual(5, self.harness.simulator.raw_remaining)

    def test_a_dropped_command_produces_no_response(self):
        # The fault a real modem produces when it is wedged: the command is accepted on the wire and
        # never answered. A client that reports success here is the bug this simulator exists to catch.
        import re

        self.harness.close()
        self.harness = PtyHarness(SimulatorConfig(drop_commands=[re.compile(r"AT\+DROP")]))
        self.addCleanup(self.harness.close)
        self.assertEqual(b"", self.harness.send(b"AT+DROP\r\n", settle_ms=120))
        self.assertIn(b"OK", self.harness.send(b"AT\r\n"))


if __name__ == "__main__":
    unittest.main()
