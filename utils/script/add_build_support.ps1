# PowerShell script to add Makefile, CMakeLists.txt, and Kconfig support to all components

# Define component descriptions
$componentDescriptions = @{
    "crypto" = "cryptographic algorithms (AES, HMAC, MD5, SHA256, etc.)"
    "xy_clib" = "custom C library for embedded systems"
    "dm" = "data management (EEPROM, Flash, TLV, etc.)"
    "net" = "network protocols (MQTT, Modbus, AT, etc.)"
    "device" = "device drivers (charger, LED, flash, etc.)"
    "trace" = "logging system"
    "osal" = "OS abstraction layer"
    "Bank" = "battery management system"
    "sensor" = "sensor drivers"
    "ipc" = "inter-process communication"
    "time_tick" = "time tick management"
    "xy_key" = "key management"
    "xy_state_machine" = "state machine framework"
    "fota" = "firmware over-the-air update"
    "kernel" = "kernel components"
    "misc" = "miscellaneous components"
    "pm" = "power management"
    "xfer" = "data transfer components"
}

# Function to process a component
function Process-Component {
    param(
        [string]$componentDir
    )

    $componentName = Split-Path $componentDir -Leaf
    Write-Host "Processing component: $componentName"

    # Skip if it's a file or hidden directory
    if ($componentName.StartsWith(".")) {
        return
    }

    # Get component description or use default
    $description = if ($componentDescriptions.ContainsKey($componentName)) {
        $componentDescriptions[$componentName]
    } else {
        "various functionalities"
    }

    # Create Makefile if it doesn't exist
    $makefilePath = Join-Path $componentDir "Makefile"
    if (-not (Test-Path $makefilePath)) {
        Write-Host "  Creating Makefile..."
        $makefileContent = Get-Content "utils\templates\Makefile.template"
        $makefileContent = $makefileContent -replace "@COMPONENT_NAME@", $componentName
        $makefileContent | Set-Content $makefilePath
    }

    # Create CMakeLists.txt if it doesn't exist
    $cmakePath = Join-Path $componentDir "CMakeLists.txt"
    if (-not (Test-Path $cmakePath)) {
        Write-Host "  Creating CMakeLists.txt..."
        $cmakeContent = Get-Content "utils\templates\CMakeLists.txt.template"
        $cmakeContent = $cmakeContent -replace "@COMPONENT_NAME@", $componentName
        $cmakeContent | Set-Content $cmakePath
    }

    # Create Kconfig if it doesn't exist
    $kconfigPath = Join-Path $componentDir "Kconfig"
    if (-not (Test-Path $kconfigPath)) {
        Write-Host "  Creating Kconfig..."
        $componentNameUC = $componentName.ToUpper()
        $kconfigContent = Get-Content "utils\templates\Kconfig.template"
        $kconfigContent = $kconfigContent -replace "@COMPONENT_NAME@", $componentName
        $kconfigContent = $kconfigContent -replace "@COMPONENT_NAME_UC@", $componentNameUC
        $kconfigContent = $kconfigContent -replace "@COMPONENT_DESCRIPTION@", $description
        $kconfigContent | Set-Content $kconfigPath
    }
}

# Main script
Write-Host "Adding build support to all components..."

# Process all components
Get-ChildItem "components" -Directory | ForEach-Object {
    Process-Component $_.FullName
}

Write-Host "Build support added to all components!"