#include "extreme_ble.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include <NimBLEAdvertising.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt_main.h"
#include "esp_bt.h"

// EXTREME-BLE UUIDs - Compatible with Flutter app
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DEVICE_INFO_UUID            "6ba7b810-9dad-11d1-80b4-00c04fd430c8"

// Legacy UUIDs for RC and Telemetry
#define EXT_BLE_RC_CHAR_UUID        "12345678-1234-5678-9abc-123456789abd"
#define EXT_BLE_TELEMETRY_CHAR_UUID "12345678-1234-5678-9abc-123456789abe"
#define EXT_BLE_BINDING_CHAR_UUID   "12345678-1234-5678-9abc-123456789abf"

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pRCCharacteristic = nullptr;
static NimBLECharacteristic* pTelemetryCharacteristic = nullptr;
static NimBLECharacteristic* pBindingCharacteristic = nullptr;
static NimBLECharacteristic* pDeviceInfoCharacteristic = nullptr;
static bool deviceConnected = false;
static SemaphoreHandle_t bleMutex = nullptr;

// Failsafe RC values (all channels centered at 1500)
static uint16_t failsafe_channels[16] = {
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500,
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500
};

// Current RC values received from mobile app
static uint16_t received_channels[16] = {
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500,
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500
};

class RCCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (value.length() == 32) { // 16 channels × 2 bytes = 32 bytes
            Serial.printf("[RC] Received 16-channel data (%d bytes)\n", value.length());

            // Parse RC data from mobile app
            for (int i = 0; i < 16; i++) {
                uint16_t channel_value = ((uint8_t)value[i*2]) | (((uint8_t)value[i*2+1]) << 8);
                received_channels[i] = channel_value;
            }

            Serial.printf("[RC] CH1=%d CH2=%d CH3=%d CH4=%d\n",
                received_channels[0], received_channels[1],
                received_channels[2], received_channels[3]);
        } else {
            Serial.printf("[RC] Invalid data length: %d bytes\n", value.length());
        }
    }
};

class BindingCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        Serial.printf("[BINDING] Received command: %s\n", value.c_str());

        String response;
        if (value == "START_BINDING") {
            Serial.println("[BINDING] Starting binding mode...");
            // Wywołaj funkcję bindingu z tx_main.cpp
            extern void EnterBindingModeSafely();
            EnterBindingModeSafely();
            response = "BINDING_STARTED";
        } else if (value == "STOP_BINDING") {
            Serial.println("[BINDING] Stopping binding mode...");
            extern void ExitBindingMode();
            ExitBindingMode();
            response = "BINDING_STOPPED";
        } else if (value == "STATUS") {
            extern bool InBindingMode;
            response = InBindingMode ? "BINDING_ACTIVE" : "BINDING_INACTIVE";
        } else {
            response = "UNKNOWN_COMMAND";
        }

        Serial.printf("[BINDING] Sending response: %s\n", response.c_str());
        pCharacteristic->setValue(response.c_str());
        pCharacteristic->notify();
    }
};

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        Serial.println("[EXTREME-BLE] Client connected");
        deviceConnected = true;
    }

    void onDisconnect(NimBLEServer* pServer) {
        Serial.println("[EXTREME-BLE] Client disconnected");
        deviceConnected = false;
        pServer->getAdvertising()->start();
    }
};

void EXT_BLE_Init(void) {
    Serial.println("[EXTREME-BLE] Initializing...");
    
    // Check BT controller status first
    esp_bt_controller_status_t bt_status = esp_bt_controller_get_status();
    if (bt_status != ESP_BT_CONTROLLER_STATUS_IDLE) {
        Serial.println("[EXTREME-BLE] BT controller busy, skipping init");
        return;
    }
    
    bleMutex = xSemaphoreCreateMutex();
    if (!bleMutex) {
        Serial.println("[EXTREME-BLE] Failed to create mutex");
        return;
    }
    
    try {
        // Get MAC address and format name
        String mac = WiFi.macAddress();
        String macSuffix = mac.substring(9, 11) + mac.substring(12, 14) + mac.substring(15, 17);
        macSuffix.toUpperCase();
        String bleName = "Extreme-Pilot " + macSuffix;
        
        NimBLEDevice::init(bleName.c_str());
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        
        pService = pServer->createService(SERVICE_UUID);
        
        // RC Data characteristic (16 channels * 2 bytes = 32 bytes)
        pRCCharacteristic = pService->createCharacteristic(
            EXT_BLE_RC_CHAR_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );
        pRCCharacteristic->setCallbacks(new RCCallbacks());
        
        // Telemetry characteristic (voltage, current, rssi, lq)
        pTelemetryCharacteristic = pService->createCharacteristic(
            EXT_BLE_TELEMETRY_CHAR_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Binding characteristic (START_BINDING, STOP_BINDING, STATUS)
        pBindingCharacteristic = pService->createCharacteristic(
            EXT_BLE_BINDING_CHAR_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );
        pBindingCharacteristic->setCallbacks(new BindingCallbacks());

        // Device info characteristic
        pDeviceInfoCharacteristic = pService->createCharacteristic(
            DEVICE_INFO_UUID,
            NIMBLE_PROPERTY::READ
        );
        pDeviceInfoCharacteristic->setValue("ExpressLRS-3.5.6-Extreme");
        
        pService->start();
        
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMaxPreferred(0x12);
        
        // Set initial failsafe values
        pRCCharacteristic->setValue((uint8_t*)failsafe_channels, 32);
        
        pAdvertising->start();
        Serial.println("[EXTREME-BLE] Started successfully!");
        
    } catch (const std::exception& e) {
        Serial.printf("[EXTREME-BLE] Init failed: %s\n", e.what());
    }
}

void EXT_BLE_PublishRC(uint16_t channels[16]) {
    if (!pRCCharacteristic || !bleMutex) return;
    
    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            pRCCharacteristic->setValue((uint8_t*)channels, 32);
            if (deviceConnected) {
                pRCCharacteristic->notify();
            }
        } catch (...) {
            // Silent fail
        }
        xSemaphoreGive(bleMutex);
    }
}

void EXT_BLE_PublishTelemetry(float voltage, float current, int8_t rssi, uint8_t lq) {
    EXT_BLE_PublishFullTelemetry(voltage, current, rssi, lq, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void EXT_BLE_PublishFullTelemetry(float voltage, float current, int8_t rssi, uint8_t lq,
                                  float speed, float heading, float lowisko, float punkt, float klapyOpenAuto, uint8_t satellites) {
    if (!pTelemetryCharacteristic || !bleMutex) return;

    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            struct {
                float voltage;
                float current;
                int8_t rssi;
                uint8_t lq;
                float speed;      // km/h from GPS
                float heading;    // degrees from GPS
                float lowisko;    // pitch from attitude (preserved mapping)
                float punkt;      // roll from attitude (preserved mapping)
                float klapyOpenAuto; // yaw from attitude (preserved mapping)
                uint8_t satellites;  // satellite count
            } tlm_data = {voltage, current, rssi, lq, speed, heading, lowisko, punkt, klapyOpenAuto, satellites};

            pTelemetryCharacteristic->setValue((uint8_t*)&tlm_data, sizeof(tlm_data));
            if (deviceConnected) {
                pTelemetryCharacteristic->notify();
            }
        } catch (...) {
            // Silent fail
        }
        xSemaphoreGive(bleMutex);
    }
}

bool EXT_BLE_IsConnected(void) {
    return deviceConnected;
}

void EXT_BLE_Task(void) {
    // Periodic housekeeping if needed
}


uint16_t* EXT_BLE_GetRCChannels(void) {
    return received_channels;
}

void EXT_BLE_StartBinding(void) {
    if (!pBindingCharacteristic || !deviceConnected) return;

    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            pBindingCharacteristic->setValue("BINDING_STARTED");
            pBindingCharacteristic->notify();
            Serial.println("[EXTREME-BLE] Binding started via API");
        } catch (...) {
            Serial.println("[EXTREME-BLE] Failed to notify binding start");
        }
        xSemaphoreGive(bleMutex);
    }
}

void EXT_BLE_StopBinding(void) {
    if (!pBindingCharacteristic || !deviceConnected) return;

    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            pBindingCharacteristic->setValue("BINDING_STOPPED");
            pBindingCharacteristic->notify();
            Serial.println("[EXTREME-BLE] Binding stopped via API");
        } catch (...) {
            Serial.println("[EXTREME-BLE] Failed to notify binding stop");
        }
        xSemaphoreGive(bleMutex);
    }
}

bool EXT_BLE_IsBinding(void) {
    extern bool InBindingMode;
    return InBindingMode;
}