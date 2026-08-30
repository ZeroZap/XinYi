#!/usr/bin/env python3
"""Guard the root Net product-selection and implementation ownership boundary."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate() -> list[str]:
    errors: list[str] = []
    kconfig = (ROOT / "Kconfig").read_text(encoding="utf-8")
    cmake = (ROOT / "components/net/CMakeLists.txt").read_text(encoding="utf-8")
    umbrella = (ROOT / "components/net/inc/xy_net.h").read_text(encoding="utf-8")
    readme = (ROOT / "components/net/README.md").read_text(encoding="utf-8")

    for block in (
        'config NETWORK\n    bool "Network Support"\n    default n',
        'config PROTO_MQTT\n    bool "MQTT Protocol"\n    depends on NETWORK\n    default n',
        'config PROTO_AT_CLIENT\n    bool "AT Client"\n    depends on NETWORK\n    default n',
        'config PROTO_AT_SERVER\n    bool "AT Server"\n    depends on NETWORK\n    default n',
        'config PROTO_CAN\n    bool "CAN Protocol"\n    depends on NETWORK\n    default n',
        'config PROTO_LTE\n    bool "LTE/4G Module"\n    depends on NETWORK && PROTO_AT_CLIENT\n    default n',
    ):
        require(block in kconfig, f"root Kconfig is missing fail-closed selection block: {block}", errors)

    for guard in (
        "if(XY_PROTO_AT_CLIENT)",
        "if(XY_PROTO_AT_SERVER)",
        "if(XY_PROTO_MQTT)",
        "if(XY_PROTO_CAN)",
        "if(XY_PROTO_LTE)",
    ):
        require(guard in cmake, f"Net source ownership is missing CMake guard: {guard}", errors)
    for definition in (
        "XY_NET_ENABLE_MQTT=$<BOOL:${XY_PROTO_MQTT}>",
        "XY_NET_ENABLE_CAN=$<BOOL:${XY_PROTO_CAN}>",
        "XY_NET_ENABLE_LTE=$<BOOL:${XY_PROTO_LTE}>",
    ):
        require(definition in cmake, f"Net public feature state is missing: {definition}", errors)

    require('#include "xy_mqtt_client.h"' in umbrella,
            "xy_net.h must export the selected active MQTT owner", errors)
    require("active AT owners are `at_client.c` and `xy_ats.c`" in readme,
            "Net README must freeze active AT ownership", errors)
    require("All Net product protocols are default-off" in readme,
            "Net README must preserve the default-off product gate", errors)
    for stale_claim in (
        "| **CAN** | ✅ Implemented |",
        "### Quick Start (AT Command V2)",
        "not auto-exported by xy_net yet",
        "Align root Kconfig (`PROTO_MQTT`)",
    ):
        require(stale_claim not in readme,
                f"Net README retains stale product-selection guidance: {stale_claim}", errors)
    require("### Quick Start (Active AT Client)" in readme,
            "Net README must demonstrate the selected AT client owner", errors)
    require("XY_NET_ENABLE_MQTT      1  // selected by CONFIG_PROTO_MQTT" in readme,
            "Net README must describe the selected MQTT umbrella export", errors)
    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("net_product_selection failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("net_product_selection_ok defaults=off mqtt=active-owner can-lte=direct-opt-in")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())