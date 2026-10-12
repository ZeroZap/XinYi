# openCH UART6 ESP8266 and UART7 CH9141 peer-response HIL

UART6 sends `AT\r\n` to the ESP8266 socket and requires an `OK` response. UART7 drives CH9141 AT low and SLEEP high, sends `AT+TPL?\r\n`, and requires a response containing `TPL`.

Boundary: this proves powered onboard peer modules, TX/RX wiring and basic AT response at 115200 baud. It does not prove Wi-Fi association, TCP/IP, BLE radio connection, throughput, DMA RX, recovery, RF performance or endurance.
