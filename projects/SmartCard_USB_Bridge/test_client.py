#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SmartCard-USB Bridge Test Client

This Python script demonstrates how to communicate with the CH32X035
SmartCard-USB Bridge device using the TLV protocol.

Author: WCH Development Team
Date: 2025/10/30
Version: 1.0.0
"""

import serial
import struct
import time
import sys

class TLVProtocol:
    """TLV Protocol Implementation"""

    # Tag Definitions
    TAG_APDU_REQUEST = 0x01
    TAG_APDU_RESPONSE = 0x02
    TAG_ATR_DATA = 0x03
    TAG_RESET_SIM = 0x04
    TAG_POWER_ON = 0x05
    TAG_POWER_OFF = 0x06
    TAG_STATUS_QUERY = 0x07
    TAG_STATUS_RESPONSE = 0x08
    TAG_ERROR = 0x09
    TAG_ACK = 0x0A
    TAG_GET_INFO = 0x0B
    TAG_INFO_RESPONSE = 0x0C
    TAG_SET_UI_INFO = 0x0D

    # Error Codes
    ERR_NONE = 0x00
    ERR_INVALID_TAG = 0x01
    ERR_INVALID_LENGTH = 0x02
    ERR_BUFFER_OVERFLOW = 0x03
    ERR_NO_CARD = 0x04
    ERR_CARD_ERROR = 0x05
    ERR_ATR_PARSE_FAILED = 0x06
    ERR_APDU_FAILED = 0x07
    ERR_TIMEOUT = 0x08
    
    # UI LED Indicator Values
    UI_LED_OFF = 0x00
    UI_LED_GREEN = 0x01
    UI_LED_RED = 0x02
    UI_LED_YELLOW = 0x03
    UI_LED_BLUE = 0x04
    UI_LED_BLINK_GREEN = 0x11
    UI_LED_BLINK_RED = 0x12
    UI_LED_BLINK_YELLOW = 0x13

    ERROR_NAMES = {
        ERR_NONE: "No Error",
        ERR_INVALID_TAG: "Invalid Tag",
        ERR_INVALID_LENGTH: "Invalid Length",
        ERR_BUFFER_OVERFLOW: "Buffer Overflow",
        ERR_NO_CARD: "No Card",
        ERR_CARD_ERROR: "Card Error",
        ERR_ATR_PARSE_FAILED: "ATR Parse Failed",
        ERR_APDU_FAILED: "APDU Failed",
        ERR_TIMEOUT: "Timeout"
    }

    @staticmethod
    def build_tlv(tag, data=b''):
        """Build TLV packet"""
        length = len(data)
        # Tag(1) + Length(2 big-endian) + Value
        return struct.pack('>BH', tag, length) + data

    @staticmethod
    def parse_tlv(buffer):
        """Parse TLV packet from buffer"""
        if len(buffer) < 3:
            return None, None, 0

        tag = buffer[0]
        length = struct.unpack('>H', buffer[1:3])[0]

        if len(buffer) < 3 + length:
            return None, None, 0

        value = buffer[3:3+length]
        total_len = 3 + length

        return tag, value, total_len

    @staticmethod
    def format_hex(data):
        """Format bytes as hex string"""
        return ' '.join([f'{b:02X}' for b in data])


class SmartCardBridge:
    """SmartCard-USB Bridge Client"""

    def __init__(self, port, baudrate=115200, timeout=2.0):
        """
        Initialize bridge client

        Args:
            port: Serial port (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
            baudrate: Baud rate (default: 115200)
            timeout: Read timeout in seconds
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.tlv = TLVProtocol()

    def connect(self):
        """Connect to the device"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            print(f"[+] Connected to {self.port} at {self.baudrate} baud")
            time.sleep(0.5)  # Wait for device to stabilize
            return True
        except Exception as e:
            print(f"[-] Failed to connect: {e}")
            return False

    def disconnect(self):
        """Disconnect from device"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("[+] Disconnected")

    def send_command(self, tag, data=b''):
        """Send TLV command"""
        tlv_packet = self.tlv.build_tlv(tag, data)
        self.ser.write(tlv_packet)
        print(f"[→] Sent: Tag=0x{tag:02X}, Len={len(data)}, Data={self.tlv.format_hex(data)}")

    def receive_response(self, expected_tag=None):
        """Receive TLV response"""
        buffer = b''
        start_time = time.time()

        while (time.time() - start_time) < self.timeout:
            if self.ser.in_waiting > 0:
                buffer += self.ser.read(self.ser.in_waiting)

                tag, value, total_len = self.tlv.parse_tlv(buffer)
                if tag is not None:
                    print(f"[←] Received: Tag=0x{tag:02X}, Len={len(value)}, Data={self.tlv.format_hex(value)}")

                    if tag == self.tlv.TAG_ERROR:
                        error_code = value[0] if len(value) > 0 else 0
                        error_name = self.tlv.ERROR_NAMES.get(error_code, "Unknown Error")
                        print(f"[!] Error: {error_name} (0x{error_code:02X})")
                        return None

                    if expected_tag is None or tag == expected_tag:
                        return value

                    buffer = buffer[total_len:]  # Remove processed packet

            time.sleep(0.01)

        print("[!] Timeout waiting for response")
        return None

    def reset_sim(self):
        """Reset SIM card and get ATR"""
        print("\n=== Resetting SIM Card ===")
        self.send_command(self.tlv.TAG_RESET_SIM)
        atr_data = self.receive_response(self.tlv.TAG_ATR_DATA)

        if atr_data and len(atr_data) > 0:
            protocol = atr_data[0]
            atr = atr_data[1:]
            print(f"[+] ATR received:")
            print(f"    Protocol: T={protocol}")
            print(f"    ATR Data: {self.tlv.format_hex(atr)}")
            return atr
        else:
            print("[-] Failed to get ATR")
            return None

    def send_apdu(self, apdu):
        """
        Send APDU command

        Args:
            apdu: APDU command as bytes or list

        Returns:
            APDU response bytes or None
        """
        if isinstance(apdu, list):
            apdu = bytes(apdu)

        print(f"\n=== Sending APDU ===")
        print(f"Command: {self.tlv.format_hex(apdu)}")

        self.send_command(self.tlv.TAG_APDU_REQUEST, apdu)
        response = self.receive_response(self.tlv.TAG_APDU_RESPONSE)

        if response and len(response) >= 2:
            data = response[:-2]
            sw1 = response[-2]
            sw2 = response[-1]

            print(f"[+] APDU Response:")
            if len(data) > 0:
                print(f"    Data: {self.tlv.format_hex(data)}")
            print(f"    SW: {sw1:02X} {sw2:02X}", end='')

            if sw1 == 0x90 and sw2 == 0x00:
                print(" (Success)")
            elif sw1 == 0x61:
                print(f" (Success, {sw2} bytes available)")
            elif sw1 == 0x6C:
                print(f" (Wrong length, correct length: {sw2})")
            else:
                print(" (Error)")

            return response
        else:
            print("[-] No response or invalid response")
            return None

    def query_status(self):
        """Query device status"""
        print("\n=== Querying Status ===")
        self.send_command(self.tlv.TAG_STATUS_QUERY)
        status = self.receive_response(self.tlv.TAG_STATUS_RESPONSE)

        if status and len(status) >= 4:
            card_state = status[0]
            active_state = status[1]
            atr_valid = status[2]
            protocol = status[3]

            print(f"[+] Status:")
            print(f"    Card State: {card_state} ({'Present' if card_state > 0 else 'Absent'})")
            print(f"    Active State: {active_state} ({'Active' if active_state > 0 else 'Inactive'})")
            print(f"    ATR Valid: {'Yes' if atr_valid else 'No'}")
            print(f"    Protocol: T={protocol}")
            return status
        else:
            print("[-] Failed to get status")
            return None

    def power_on(self):
        """Power on SIM card"""
        print("\n=== Powering ON SIM Card ===")
        self.send_command(self.tlv.TAG_POWER_ON)
        response = self.receive_response(self.tlv.TAG_ACK)
        return response is not None

    def power_off(self):
        """Power off SIM card"""
        print("\n=== Powering OFF SIM Card ===")
        self.send_command(self.tlv.TAG_POWER_OFF)
        response = self.receive_response(self.tlv.TAG_ACK)
        return response is not None

    def get_info(self):
        """Get card information"""
        print("\n=== Getting Card Info ===")
        self.send_command(self.tlv.TAG_GET_INFO)
        info = self.receive_response(self.tlv.TAG_INFO_RESPONSE)

        if info and len(info) >= 2:
            protocol = info[0]
            hist_bytes_count = info[1]
            print(f"[+] Card Info:")
            print(f"    Protocol: T={protocol}")
            print(f"    Historical Bytes Count: {hist_bytes_count}")
            return info
        else:
            print("[-] Failed to get info")
            return None
    
    def set_ui_led(self, led_state):
        """Set UI LED indicator
        
        Args:
            led_state: LED state value (UI_LED_xxx constants)
        
        Returns:
            True if successful, False otherwise
        """
        print(f"\n=== Setting UI LED: 0x{led_state:02X} ===")
        
        led_names = {
            self.tlv.UI_LED_OFF: "OFF",
            self.tlv.UI_LED_GREEN: "GREEN",
            self.tlv.UI_LED_RED: "RED",
            self.tlv.UI_LED_YELLOW: "YELLOW",
            self.tlv.UI_LED_BLUE: "BLUE",
            self.tlv.UI_LED_BLINK_GREEN: "BLINK_GREEN",
            self.tlv.UI_LED_BLINK_RED: "BLINK_RED",
            self.tlv.UI_LED_BLINK_YELLOW: "BLINK_YELLOW"
        }
        
        led_name = led_names.get(led_state, f"Unknown (0x{led_state:02X})")
        print(f"LED State: {led_name}")
        
        self.send_command(self.tlv.TAG_SET_UI_INFO, bytes([led_state]))
        response = self.receive_response(self.tlv.TAG_ACK)
        
        if response is not None:
            if len(response) > 0:
                confirmed_state = response[0]
                print(f"[+] LED set successfully. Confirmed state: 0x{confirmed_state:02X}")
            else:
                print("[+] LED set successfully")
            return True
        else:
            print("[-] Failed to set LED")
            return False


def main():
    """Main test program"""
    print("=" * 60)
    print("SmartCard-USB Bridge Test Client")
    print("Version 1.0.0")
    print("=" * 60)

    # Configure your serial port here
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = 'COM3'  # Change this to your port (e.g., '/dev/ttyUSB0' on Linux)

    # Create bridge client
    bridge = SmartCardBridge(port)

    if not bridge.connect():
        return

    try:
        # Test sequence
        time.sleep(1)

        # 1. Query initial status
        bridge.query_status()
        
        # 1.5 Test LED controls
        print("\n" + "=" * 60)
        print("LED Control Test")
        print("=" * 60)
        bridge.set_ui_led(bridge.tlv.UI_LED_BLUE)
        time.sleep(0.5)
        bridge.set_ui_led(bridge.tlv.UI_LED_YELLOW)
        time.sleep(0.5)

        # 2. Reset SIM card and get ATR
        atr = bridge.reset_sim()
        if not atr:
            print("\n[-] Cannot continue without ATR")
            return

        # 3. Query status after reset
        bridge.query_status()

        # 4. Send some common APDU commands

        # SELECT MF (Master File)
        print("\n" + "=" * 60)
        print("Test 1: SELECT MF (Master File)")
        print("=" * 60)
        apdu_select_mf = [0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00]
        bridge.send_apdu(apdu_select_mf)

        # GET CHALLENGE
        print("\n" + "=" * 60)
        print("Test 2: GET CHALLENGE")
        print("=" * 60)
        apdu_get_challenge = [0x00, 0x84, 0x00, 0x00, 0x08]
        bridge.send_apdu(apdu_get_challenge)

        # SELECT EF_ICCID (2FE2)
        print("\n" + "=" * 60)
        print("Test 3: SELECT EF_ICCID")
        print("=" * 60)
        apdu_select_iccid = [0x00, 0xA4, 0x00, 0x00, 0x02, 0x2F, 0xE2]
        bridge.send_apdu(apdu_select_iccid)

        # READ BINARY ICCID
        print("\n" + "=" * 60)
        print("Test 4: READ BINARY (ICCID)")
        print("=" * 60)
        apdu_read_binary = [0x00, 0xB0, 0x00, 0x00, 0x0A]
        response = bridge.send_apdu(apdu_read_binary)

        if response and len(response) > 2:
            iccid_data = response[:-2]
            print(f"\n[+] ICCID (BCD): {TLVProtocol.format_hex(iccid_data)}")
            # Convert BCD to string
            iccid_str = ''.join([f'{b:02X}' for b in iccid_data])
            # Swap nibbles for proper ICCID format
            iccid_str = ''.join([iccid_str[i+1] + iccid_str[i] for i in range(0, len(iccid_str), 2)])
            print(f"[+] ICCID (Decoded): {iccid_str}")

        # Final status query
        bridge.query_status()
        
        # Final LED test - set to GREEN (ready)
        print("\n" + "=" * 60)
        print("Final LED State: GREEN (Ready)")
        print("=" * 60)
        bridge.set_ui_led(bridge.tlv.UI_LED_GREEN)

        print("\n" + "=" * 60)
        print("All tests completed!")
        print("=" * 60)

    except KeyboardInterrupt:
        print("\n[!] Interrupted by user")
    except Exception as e:
        print(f"\n[!] Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        bridge.disconnect()


if __name__ == '__main__':
    main()
