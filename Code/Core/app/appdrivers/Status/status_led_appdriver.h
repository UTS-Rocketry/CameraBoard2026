#ifndef STATUS_LED_APPDRIVER_H
#define STATUS_LED_APPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    STATUS_LED_STATE_OFF = 0,
    STATUS_LED_STATE_BOOTING,
    STATUS_LED_STATE_IDLE,
    STATUS_LED_STATE_ARMED,
    STATUS_LED_STATE_RECORDING,
    STATUS_LED_STATE_ERROR
} StatusLed_State_t;

void StatusLed_AppDriver_Init(void);
void StatusLed_AppDriver_SetState(StatusLed_State_t state);
StatusLed_State_t StatusLed_AppDriver_GetState(void);
void StatusLed_AppDriver_Set(bool enabled);
void StatusLed_AppDriver_Toggle(void);
bool StatusLed_AppDriver_IsOn(void);
void StatusLed_AppDriver_Task(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* STATUS_LED_APPDRIVER_H */
