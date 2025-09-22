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
cd /home/projekty/66_elrs_done
./build_elrs_extreme.sh
```

**Skrypt automatycznie:**
1. Kompiluje firmware z EXTREME-BLE modyfikacjami
2. Wykonuje binary patching dla RadioMaster Ranger Nano
3. Wdraża pliki na serwer WWW
4. Generuje instrukcje flashowania

### Ręczne budowanie
```bash
cd ExpressLRS/src
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
- **ExpressLRS**: `./ExpressLRS/src` (local)
- **Hardware**: `./hardware` (local)
- **Target**: `Unified_ESP32_2400_TX_via_UART`
- **Device**: `RadioMaster_TX_Ranger_Nano_2400`

**⚠️ Projekt jest samodzielny - wszystkie zależności w lokalnych katalogach**

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
- **BLE Library**: `./ExpressLRS/src/lib/BLE/extreme_ble.{h,cpp}`
- **TX Main**: Modified `./ExpressLRS/src/src/tx_main.cpp`
- **Options**: Updated `./ExpressLRS/src/lib/OPTIONS/options.cpp`

### Binary Configurator Settings
**Kluczowe parametry (wymagane dla poprawnego działania):**
```bash
python3 python/binary_configurator.py \
    firmware.bin \
    --target "radiomaster.tx_2400.ranger-nano" \
    --phrase "" \
    --lbt \
    --auto-wifi 90
```

**⚠️ UWAGA**: `--auto-wifi 90` jest WYMAGANE! Bez tego WiFi nie włączy się po 90s.

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

## 🔧 Troubleshooting

### ❌ WiFi nie włącza się po 90 sekundach
**Problem**: Brak WiFi "Extreme-Update" po 90s bezczynności
**Rozwiązanie**:
```bash
# Sprawdź czy firmware ma poprawną konfigurację
strings /var/www/html/elrs/firmware.bin | grep "wifi-on-interval"
# Powinno pokazać: {"wifi-on-interval": 90, ...}

# Jeśli brak, przebuduj z --auto-wifi:
./build_elrs_extreme.sh
```

### ❌ Błąd "Regulatory_Domain 2400 not compatible"
**Problem**: `--domain fcc_915` dla 2400MHz
**Rozwiązanie**: Użyj `--lbt` zamiast `--domain` dla SX1280 (2400MHz)

### ❌ Błąd "Target not found: RadioMaster Ranger Nano"
**Problem**: Nieprawidłowa nazwa targetu
**Rozwiązanie**: Używaj **dokładnie**: `radiomaster.tx_2400.ranger-nano`

### ❌ Binary configurator "file not found"
**Problem**: Brak pliku `hardware/targets.json`
**Rozwiązanie**:
```bash
# Uruchom z katalogu zawierającego hardware/
cd /path/to/project
python3 ExpressLRS/src/python/binary_configurator.py ...
```

### 🔍 Diagnostyka
**Sprawdź skompilowane build flags:**
```bash
# Powinno zawierać -DAUTO_WIFI_ON_INTERVAL=90
grep -A 5 "build flags:" build_log.txt
```

**Sprawdź JSON w firmware:**
```bash
strings firmware.bin | grep wifi-on-interval
```

## 📋 Wersjonowanie i Historia

### v1.0 - Podstawowa implementacja (Sep 2024)
- EXTREME-BLE integration
- WiFi timeout 90s
- LBT regulatory compliance

### v1.1 - Standalone project (Sep 2024)
- Self-contained with all dependencies
- Local ExpressLRS and hardware sources
- Updated paths and portability
- Complete troubleshooting documentation

---

**Created for**: Autonomous boat control systems
**Compatible with**: AlfredoCRSF + ExpressLRS 3.5.6+
**License**: Follows ExpressLRS GPL-3.0 license