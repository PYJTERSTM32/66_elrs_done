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

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pRCCharacteristic = nullptr;
static NimBLECharacteristic* pTelemetryCharacteristic = nullptr;
// New characteristics for ping-pong test
static NimBLECharacteristic* pPingPongCharacteristic = nullptr;
static NimBLECharacteristic* pDeviceInfoCharacteristic = nullptr;
static bool deviceConnected = false;
static SemaphoreHandle_t bleMutex = nullptr;

// Failsafe RC values (all channels centered at 1500)
static uint16_t failsafe_channels[16] = {
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500,
    1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500
};

class PingPongCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        String value = pCharacteristic->getValue().c_str();
        Serial.printf("[PING-PONG] Received: %s\n", value.c_str());
        
        // Ping-pong response logic
        String response;
        if (value == "PING") {
            response = "PONG";
        } else if (value == "TEST") {
            response = "OK";
        } else if (value == "STATUS") {
            response = "READY";
        } else if (value.startsWith("ECHO:")) {
            response = "ECHO_REPLY:" + value.substring(5);
        } else {
            response = "UNKNOWN_CMD";
        }
        
        Serial.printf("[PING-PONG] Sending response: %s\n", response.c_str());
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
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        
        // Telemetry characteristic (voltage, current, rssi, lq)
        pTelemetryCharacteristic = pService->createCharacteristic(
            EXT_BLE_TELEMETRY_CHAR_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        
        // Ping-Pong test characteristic for Flutter app communication
        pPingPongCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );
        pPingPongCharacteristic->setCallbacks(new PingPongCallbacks());
        
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
    if (!pTelemetryCharacteristic || !bleMutex) return;
    
    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            struct {
                float voltage;
                float current;
                int8_t rssi;
                uint8_t lq;
            } tlm_data = {voltage, current, rssi, lq};
            
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

void EXT_BLE_SendPingPongMessage(const char* message) {
    if (!pPingPongCharacteristic || !deviceConnected) return;
    
    if (xSemaphoreTake(bleMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        try {
            pPingPongCharacteristic->setValue(message);
            pPingPongCharacteristic->notify();
            Serial.printf("[PING-PONG] Sent: %s\n", message);
        } catch (...) {
            Serial.println("[PING-PONG] Failed to send message");
        }
        xSemaphoreGive(bleMutex);
    }
}

void EXT_BLE_TestCommunication(void) {
    static uint32_t lastTest = 0;
    uint32_t now = millis();
    
    // Send test message every 10 seconds if connected
    if (deviceConnected && (now - lastTest > 10000)) {
        EXT_BLE_SendPingPongMessage("HEARTBEAT");
        lastTest = now;
    }
}