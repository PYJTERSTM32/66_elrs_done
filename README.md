# 🔵 EXTREME-BLE ExpressLRS Build System

Automatyczny system budowania firmware ExpressLRS z integracją EXTREME-BLE dla RadioMaster Ranger Nano.

## 🎯 Funkcjonalność

### EXTREME-BLE Features
- **BLE RC Control**: 16-kanałowy CRSF przez Bluetooth @50Hz
- **BLE Telemetria**: Wysyłanie danych telemetrycznych @1Hz  
- **WiFi Extended**: 90s timeout zamiast standardowych 60s
- **Custom Names**: 
  - WiFi: `Extreme-Update`
  - BLE: `Extreme-Pilot XXXXXX` (MAC suffix)

### Hardware Target
- **Device**: RadioMaster Ranger Nano 2400MHz
- **Platform**: ESP32 
- **Regulatory**: LBT (Listen Before Talk) EU CE 2400
- **Protocol**: ExpressLRS 3.5.6+ compatible

## 🚀 Użycie

### Automatyczne budowanie
```bash
cd /home/projekty/50_elrs_build
./build_elrs_extreme.sh
```

### Ręczne budowanie  
```bash
cd /tmp/ExpressLRS/src
pio run -e Unified_ESP32_2400_TX_via_UART
```

### Flashowanie
```bash
esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash 0x1000 /var/www/html/elrs/bootloader.bin 0x8000 /var/www/html/elrs/partitions.bin 0xe000 /var/www/html/elrs/boot_app0.bin 0x10000 /var/www/html/elrs/firmware.bin
```

## 📁 Struktura Output

### Firmware Files
```
/var/www/html/elrs/
├── bootloader.bin      # ESP32 Bootloader
├── partitions.bin      # Partition Table  
├── boot_app0.bin       # Boot Application
├── firmware.bin        # EXTREME-BLE Firmware
└── index.html          # Web Interface
```

### Mobile App
```  
/var/www/html/publish/
└── extreme-ble-crsf-mobile.apk  # Android Controller
```

## 🔧 Konfiguracja

### Source Locations
- **ExpressLRS**: `/tmp/ExpressLRS/src`
- **Hardware**: `/tmp/hardware`
- **Target**: `Unified_ESP32_2400_TX_via_UART`
- **Device**: `RadioMaster_TX_Ranger_Nano_2400`

### Build Flags
```cpp
-DPLATFORM_ESP32=1
-DRADIO_SX128X=1
-DTARGET_TX=1
-DLOCK_ON_FIRST_CONNECTION
-DAUTO_WIFI_ON_INTERVAL=90
-DRegulatory_Domain_EU_CE_2400
```

### Custom Code Integration
- **BLE Library**: `/tmp/ExpressLRS/src/lib/BLE/extreme_ble.{h,cpp}`
- **TX Main**: Modified `/tmp/ExpressLRS/src/src/tx_main.cpp`
- **Options**: Updated `/tmp/ExpressLRS/src/lib/OPTIONS/options.cpp`

## 📱 Aplikacja Mobilna

### Funkcjonalność
- **Virtual Joystick**: CH1 (Roll), CH2 (Pitch) control
- **BLE Connection**: Auto-scan for `Extreme-Pilot` devices
- **Real-time RC**: 50Hz channel data transmission
- **GPS Telemetry**: Position, speed, heading @1Hz
- **Live Display**: RC values, connection status, telemetry

### Kompilacja APK
```bash
cd /home/projekty/70_crsf_mobile
flutter build apk --release
```

## 🌐 Web Access

- **Firmware**: http://SERVER_IP/elrs/
- **Mobile App**: http://SERVER_IP/publish/extreme-ble-crsf-mobile.apk
- **Build Status**: Console output podczas budowania

## ⚡ Performance

### Latency
- **Joystick → BLE**: <5ms
- **BLE → EXTREME-BLE**: <10ms  
- **Total RC**: <20ms (excellent for RC control)

### Throughput
- **RC Data**: 32 bytes @50Hz = 1.6KB/s
- **Telemetry**: 10 bytes @1Hz = 10B/s
- **BLE Overhead**: Minimal impact

## 🔗 Integration

### AlfredoCRSF Library
- **CRSF Frames**: Standard 16-channel support
- **Telemetry**: GPS position, battery voltage
- **Failsafe**: Automatic neutral values (1500μs)

### ExpressLRS Protocol  
- **Standalone Mode**: Independent of RF link
- **LBT Compliance**: EU regulatory compliant
- **Power Management**: Standard ELRS power levels

---

**Created for**: Autonomous boat control systems  
**Compatible with**: AlfredoCRSF + ExpressLRS 3.5.6+  
**License**: Follows ExpressLRS GPL-3.0 license