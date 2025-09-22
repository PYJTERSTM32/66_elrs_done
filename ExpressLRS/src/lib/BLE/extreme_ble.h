#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// EXTREME-BLE API
void EXT_BLE_Init(void);
void EXT_BLE_PublishRC(uint16_t channels[16]);
void EXT_BLE_PublishTelemetry(float voltage, float current, int8_t rssi, uint8_t lq);
void EXT_BLE_PublishFullTelemetry(float voltage, float current, int8_t rssi, uint8_t lq,
                                  float speed, float heading, float lowisko, float punkt, float klapyOpenAuto, uint8_t satellites);
bool EXT_BLE_IsConnected(void);
void EXT_BLE_Task(void);
// RC channel reception
uint16_t* EXT_BLE_GetRCChannels(void);
// Binding functions
void EXT_BLE_StartBinding(void);
void EXT_BLE_StopBinding(void);
bool EXT_BLE_IsBinding(void);

#ifdef __cplusplus
}
#endif