# openCH CH32V307 RTC LSI runtime evidence

The image was programmed, verified and reset through WCH-Link. The smoke configures the RTC from the internal LSI, writes and reads a 32-bit backup-register value, then observes the RTC counter increasing. This is bounded runtime evidence only: LSI frequency is nominal and uncalibrated, no external 32.768 kHz crystal was established, and alarm IRQ, retention across power loss, drift and long-duration accuracy are not qualified.
