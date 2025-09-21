#!/bin/bash

# EXTREME-BLE ExpressLRS Build Script
# Compiles and patches firmware for Ranger Nano with EXTREME-BLE features

set -e

echo "🚀 Building EXTREME-BLE ExpressLRS firmware..."

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ELRS_PATH="$SCRIPT_DIR/ExpressLRS/src"
HARDWARE_PATH="$SCRIPT_DIR/hardware"
TARGET_ENV="Unified_ESP32_2400_TX_via_UART"
DEVICE_NAME="RadioMaster_TX_Ranger_Nano_2400"
OUTPUT_DIR="/var/www/html/elrs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m' 
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
print_step "Checking prerequisites..."
if [ ! -d "$ELRS_PATH" ]; then
    print_error "ExpressLRS source not found at $ELRS_PATH"
    exit 1
fi

if [ ! -d "$HARDWARE_PATH" ]; then
    print_error "Hardware definitions not found at $HARDWARE_PATH"
    exit 1
fi

if ! command -v pio &> /dev/null; then
    print_error "PlatformIO not found. Please install PlatformIO."
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    print_error "Python3 not found. Please install Python3."
    exit 1
fi

print_success "Prerequisites check completed"

# Show build configuration
print_step "Build Configuration:"
echo "  - Source: $ELRS_PATH"
echo "  - Target: $TARGET_ENV"  
echo "  - Device: $DEVICE_NAME"
echo "  - Output: $OUTPUT_DIR"
echo "  - Features:"
echo "    ✓ EXTREME-BLE integration"
echo "    ✓ WiFi timeout: 90s"
echo "    ✓ WiFi SSID: Extreme-Update"
echo "    ✓ BLE name: Extreme-Pilot XXXXXX"
echo "    ✓ LBT regulatory domain"

# Build firmware
print_step "Compiling firmware..."
cd "$ELRS_PATH"
timeout 300 pio run -e "$TARGET_ENV"
if [ $? -ne 0 ]; then
    print_error "Firmware compilation failed"
    exit 1
fi
print_success "Firmware compiled successfully"

# Get binary paths
FIRMWARE_BIN="$ELRS_PATH/.pio/build/$TARGET_ENV/firmware.bin"
BOOTLOADER_BIN="$ELRS_PATH/.pio/build/$TARGET_ENV/bootloader.bin"
PARTITIONS_BIN="$ELRS_PATH/.pio/build/$TARGET_ENV/partitions.bin"

# Verify binaries exist
for bin in "$FIRMWARE_BIN" "$BOOTLOADER_BIN" "$PARTITIONS_BIN"; do
    if [ ! -f "$bin" ]; then
        print_error "Binary file not found: $bin"
        exit 1
    fi
done

# Patch firmware
print_step "Patching firmware with hardware configuration..."
cd "$HARDWARE_PATH/.."
python3 "$ELRS_PATH/python/binary_configurator.py" \
    "$FIRMWARE_BIN" \
    --target "radiomaster.tx_2400.ranger-nano" \
    --phrase "" \
    --lbt \
    --auto-wifi 90

if [ $? -ne 0 ]; then
    print_error "Firmware patching failed"
    exit 1
fi

# Binary configurator modifies firmware.bin in place
PATCHED_FIRMWARE="$FIRMWARE_BIN"
print_success "Firmware patched successfully"

# Create output directory
print_step "Preparing deployment..."
sudo mkdir -p "$OUTPUT_DIR"
sudo chown $(whoami):$(whoami) "$OUTPUT_DIR"

# Deploy binaries
print_step "Deploying firmware files..."

# Copy and rename binaries
cp "$BOOTLOADER_BIN" "$OUTPUT_DIR/bootloader.bin"
cp "$PARTITIONS_BIN" "$OUTPUT_DIR/partitions.bin"  
cp "$ELRS_PATH/.pio/build/$TARGET_ENV/boot_app0.bin" "$OUTPUT_DIR/boot_app0.bin"
cp "$PATCHED_FIRMWARE" "$OUTPUT_DIR/firmware.bin"

# Verify deployment
for file in "bootloader.bin" "partitions.bin" "boot_app0.bin" "firmware.bin"; do
    if [ ! -f "$OUTPUT_DIR/$file" ]; then
        print_error "Deployment failed: $file not found"
        exit 1
    fi
    SIZE=$(stat -c%s "$OUTPUT_DIR/$file")
    print_success "Deployed $file (${SIZE} bytes)"
done

# Generate flash command
FLASH_CMD="esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash"
FLASH_CMD+=" 0x1000 $OUTPUT_DIR/bootloader.bin"
FLASH_CMD+=" 0x8000 $OUTPUT_DIR/partitions.bin" 
FLASH_CMD+=" 0xe000 $OUTPUT_DIR/boot_app0.bin"
FLASH_CMD+=" 0x10000 $OUTPUT_DIR/firmware.bin"

# Save flash command
echo "$FLASH_CMD" > "$OUTPUT_DIR/flash_command.txt"
print_success "Flash command saved to $OUTPUT_DIR/flash_command.txt"

# Create web-friendly index
cat > "$OUTPUT_DIR/index.html" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>EXTREME-BLE Firmware</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }
        .file-list { list-style: none; padding: 0; }
        .file-list li { margin: 8px 0; padding: 10px; background: #ecf0f1; border-radius: 4px; }
        .file-list a { text-decoration: none; color: #2980b9; font-weight: bold; }
        .info { background: #d4edda; border: 1px solid #c3e6cb; padding: 15px; border-radius: 4px; margin: 20px 0; }
        .flash-cmd { background: #f8f9fa; border: 1px solid #dee2e6; padding: 15px; border-radius: 4px; font-family: monospace; font-size: 12px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔵 EXTREME-BLE Firmware Files</h1>
        
        <div class="info">
            <strong>Build Date:</strong> $(date)<br>
            <strong>Target:</strong> RadioMaster Ranger Nano 2400MHz<br>
            <strong>Features:</strong> EXTREME-BLE, LBT, 90s WiFi timeout, Extreme-Update WiFi, Extreme-Pilot BLE
        </div>

        <h2>📁 Firmware Files</h2>
        <ul class="file-list">
            <li><a href="bootloader.bin">bootloader.bin</a> - ESP32 Bootloader</li>
            <li><a href="partitions.bin">partitions.bin</a> - Partition Table</li>
            <li><a href="boot_app0.bin">boot_app0.bin</a> - Boot Application</li>
            <li><a href="firmware.bin">firmware.bin</a> - EXTREME-BLE Firmware</li>
        </ul>

        <h2>⚡ Flash Command</h2>
        <div class="flash-cmd">
$FLASH_CMD
        </div>

        <h2>📱 Mobile App</h2>
        <ul class="file-list">
            <li><a href="../publish/extreme-ble-crsf-mobile.apk">extreme-ble-crsf-mobile.apk</a> - Android Controller App</li>
        </ul>
    </div>
</body>
</html>
EOF

print_success "Web interface created at $OUTPUT_DIR/index.html"

# Display summary
echo
echo "=================================="
echo "🎉 EXTREME-BLE BUILD COMPLETED"
echo "=================================="
echo "📁 Files deployed to: $OUTPUT_DIR"
echo "🌐 Web access: http://$(hostname -I | awk '{print $1}')/elrs/"
echo "📱 Mobile app: http://$(hostname -I | awk '{print $1}')/publish/extreme-ble-crsf-mobile.apk"
echo
echo "🔧 Flash command:"
echo "$FLASH_CMD"
echo
echo "🚀 Ready to flash RadioMaster Ranger Nano!"