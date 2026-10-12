# openCH W25Q128 reversible sector R/W HIL

Backs up sector `0x00EFF000..0x00EFFFFF`, erases it, programs and reads back a 256-byte deterministic pattern, then erases and restores the complete original 4 KiB and verifies the restored bytes. The sector is below the FOTA candidate boundary at `0x00F00000`.

Boundary: this proves one reversible sector transaction. It does not prove LittleFS mount/format/file behavior, power-loss recovery, retention, endurance, quad mode or the FOTA partition contents.
