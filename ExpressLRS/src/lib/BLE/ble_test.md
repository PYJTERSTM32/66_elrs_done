# BLE Ping-Pong Test Communication

## Overview
Prosty system testowania komunikacji BLE między nadajnikiem ExpressLRS a aplikacją Flutter.

## UUIDs Compatible with Flutter App
- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Ping-Pong Characteristic**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Device Info Characteristic**: `6ba7b810-9dad-11d1-80b4-00c04fd430c8`

## Supported Commands

### From Flutter App to ExpressLRS:
1. **"PING"** → Response: **"PONG"**
2. **"TEST"** → Response: **"OK"** 
3. **"STATUS"** → Response: **"READY"**
4. **"ECHO:message"** → Response: **"ECHO_REPLY:message"**
5. **Any other** → Response: **"UNKNOWN_CMD"**

### From ExpressLRS to Flutter App:
- **"HEARTBEAT"** - Wysyłany co 10 sekund gdy połączony

## Device Name
Nadajnik będzie widoczny jako: **"Extreme-Pilot XXXXXX"** gdzie XXXXXX to ostatnie 6 znaków MAC adresu.

## Usage in Flutter
1. Skanuj urządzenia BLE szukając nazwy "Extreme-Pilot"
2. Połącz się z urządzeniem
3. Znajdź service o UUID `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
4. Znajdź characteristic o UUID `beb5483e-36e1-4688-b7f5-ea07361b26a8`
5. Włącz notifications na tym characteristic
6. Wysyłaj komendy i odbieraj odpowiedzi

## Testing Steps
1. Flash firmware na nadajnik
2. Uruchom aplikację Flutter 
3. Połącz się z "Extreme-Pilot XXXXXX"
4. Wyślij "PING" - powinieneś otrzymać "PONG"
5. Co 10 sekund otrzymasz "HEARTBEAT"

## Serial Monitor Output
Wszystkie komunikaty będą logowane na Serial Monitor z prefiksami:
- `[PING-PONG] Received: ...`
- `[PING-PONG] Sending response: ...`
- `[PING-PONG] Sent: ...`