# openCH CH32V307 internal Flash erase/program/readback evidence

The dedicated smoke linker script limits the application to the first 284 KiB and reserves the final 4 KiB page at `0x08047000`. The image repeatedly unlocks, erases that page, programs a fixed 16-byte pattern, locks, reads it back, and compares every byte. This page is reserved only for this destructive smoke. Evidence does not establish power-loss recovery, protection/RDP, mass erase, endurance, retention, or a production partition contract.
