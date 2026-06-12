# CH32V QEMU Tests

This directory contains minimal CH32V/QingKe QEMU smoke tests for `qemu-ch32v`.

## Requirements

- `qemu-system-riscv32` from `https://github.com/lintel/qemu-ch32v`
- WCH RISC-V toolchain, default path:
  `/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin`

## Usage

```bash
make -C tests/qemu_ch32v test
```

Override paths if needed:

```bash
make -C tests/qemu_ch32v test \
  QEMU=/path/to/qemu-system-riscv32 \
  WCH_TOOLCHAIN_BIN="/path/to/wch/toolchain/bin"
```

The default machine is `ch32v307`.
