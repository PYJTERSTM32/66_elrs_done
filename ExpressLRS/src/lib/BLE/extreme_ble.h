#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// EXTREME-BLE API
void EXT_BLE_Init(void);
void EXT_BLE_PublishRC(uint16_t channels[16]);
void EXT_BLE_PublishTelemetry(float voltage, float current, int8_t rssi, uint8_t lq);
bool EXT_BLE_IsConnected(void);
void EXT_BLE_Task(void);
// New ping-pong test functions
void EXT_BLE_SendPingPongMessage(const char* message);
void EXT_BLE_TestCommunication(void);

#ifdef __cplusplus
}
#endif