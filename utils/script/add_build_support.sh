#!/bin/bash
# Script to add Makefile, CMakeLists.txt, and Kconfig support to all components

# Define component descriptions
declare -A COMPONENT_DESCRIPTIONS
COMPONENT_DESCRIPTIONS["crypto"]="cryptographic algorithms (AES, HMAC, MD5, SHA256, etc.)"
COMPONENT_DESCRIPTIONS["xy_clib"]="custom C library for embedded systems"
COMPONENT_DESCRIPTIONS["dm"]="data management (EEPROM, Flash, TLV, etc.)"
COMPONENT_DESCRIPTIONS["net"]="network protocols (MQTT, Modbus, AT, etc.)"
COMPONENT_DESCRIPTIONS["device"]="device drivers (charger, LED, flash, etc.)"
COMPONENT_DESCRIPTIONS["trace"]="logging system"
COMPONENT_DESCRIPTIONS["osal"]="OS abstraction layer"
COMPONENT_DESCRIPTIONS["Bank"]="battery management system"
COMPONENT_DESCRIPTIONS["sensor"]="sensor drivers"
COMPONENT_DESCRIPTIONS["ipc"]="inter-process communication"
COMPONENT_DESCRIPTIONS["time_tick"]="time tick management"
COMPONENT_DESCRIPTIONS["xy_key"]="key management"
COMPONENT_DESCRIPTIONS["xy_state_machine"]="state machine framework"
COMPONENT_DESCRIPTIONS["fota"]="firmware over-the-air update"
COMPONENT_DESCRIPTIONS["kernel"]="kernel components"
COMPONENT_DESCRIPTIONS["misc"]="miscellaneous components"
COMPONENT_DESCRIPTIONS["pm"]="power management"
COMPONENT_DESCRIPTIONS["xfer"]="data transfer components"

# Function to process a component
process_component() {
    local component_dir="$1"
    local component_name=$(basename "$component_dir")

    echo "Processing component: $component_name"

    # Skip if it's a file or hidden directory
    if [[ ! -d "$component_dir" ]] || [[ "$component_name" == .* ]]; then
        return
    fi

    # Get component description or use default
    local description="${COMPONENT_DESCRIPTIONS[$component_name]:-"various functionalities"}"

    # Create Makefile if it doesn't exist
    if [[ ! -f "$component_dir/Makefile" ]]; then
        echo "  Creating Makefile..."
        sed -e "s/@COMPONENT_NAME@/$component_name/g" \
            "utils/templates/Makefile.template" > "$component_dir/Makefile"
    fi

    # Create CMakeLists.txt if it doesn't exist
    if [[ ! -f "$component_dir/CMakeLists.txt" ]]; then
        echo "  Creating CMakeLists.txt..."
        sed -e "s/@COMPONENT_NAME@/$component_name/g" \
            "utils/templates/CMakeLists.txt.template" > "$component_dir/CMakeLists.txt"
    fi

    # Create Kconfig if it doesn't exist
    if [[ ! -f "$component_dir/Kconfig" ]]; then
        echo "  Creating Kconfig..."
        local component_name_uc=$(echo "$component_name" | tr '[:lower:]' '[:upper:]')
        sed -e "s/@COMPONENT_NAME@/$component_name/g" \
            -e "s/@COMPONENT_NAME_UC@/$component_name_uc/g" \
            -e "s/@COMPONENT_DESCRIPTION@/$description/g" \
            "utils/templates/Kconfig.template" > "$component_dir/Kconfig"
    fi
}

# Main script
echo "Adding build support to all components..."

# Process all components
for component in components/*/; do
    process_component "$component"
done

echo "Build support added to all components!"