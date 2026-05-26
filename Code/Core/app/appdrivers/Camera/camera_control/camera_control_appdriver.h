#ifndef CAMERA_CONTROL_APPDRIVER_H
#define CAMERA_CONTROL_APPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define CAMERA_CONTROL_MAX_PACKET_LEN 16U
#define CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS 50U
#define CAMERA_CONTROL_DEFAULT_BAUD_RATE 9600U

typedef enum
{
    CAMERA_CONTROL_STATUS_OK = 0,
    CAMERA_CONTROL_STATUS_NOT_INITIALIZED,
    CAMERA_CONTROL_STATUS_INVALID_ARGUMENT,
    CAMERA_CONTROL_STATUS_COMMAND_NOT_CONFIGURED,
    CAMERA_CONTROL_STATUS_UNSUPPORTED_UART,
    CAMERA_CONTROL_STATUS_UART_NOT_ENABLED,
    CAMERA_CONTROL_STATUS_UART_TIMEOUT
} CameraControl_Status_t;

typedef enum
{
    CAMERA_CONTROL_COMMAND_START_RECORDING = 0,
    CAMERA_CONTROL_COMMAND_STOP_RECORDING,
    CAMERA_CONTROL_COMMAND_TOGGLE_RECORDING,
    CAMERA_CONTROL_COMMAND_SET_VIDEO_MODE,
    CAMERA_CONTROL_COMMAND_SET_PHOTO_MODE,
    CAMERA_CONTROL_COMMAND_POWER_BUTTON,
    CAMERA_CONTROL_COMMAND_MODE_BUTTON,
    CAMERA_CONTROL_COMMAND_COUNT
} CameraControl_Command_t;

typedef struct
{
    USART_TypeDef *uart;
    uint32_t baud_rate;
    uint32_t tx_timeout_ms;
    bool configure_uart;
} CameraControl_Config_t;

typedef struct
{
    uint8_t bytes[CAMERA_CONTROL_MAX_PACKET_LEN];
    uint8_t length;
} CameraControl_Packet_t;

CameraControl_Status_t CameraControl_AppDriver_Init(const CameraControl_Config_t *config);
void CameraControl_AppDriver_DeInit(void);
bool CameraControl_AppDriver_IsReady(void);
CameraControl_Status_t CameraControl_AppDriver_SetCommandPacket(CameraControl_Command_t command,
                                                                 const uint8_t *bytes,
                                                                 uint8_t length);
CameraControl_Status_t CameraControl_AppDriver_ClearCommandPacket(CameraControl_Command_t command);
CameraControl_Status_t CameraControl_AppDriver_SendCommand(CameraControl_Command_t command);
CameraControl_Status_t CameraControl_AppDriver_SendRaw(const uint8_t *bytes, uint8_t length);
CameraControl_Status_t CameraControl_AppDriver_StartRecording(void);
CameraControl_Status_t CameraControl_AppDriver_StopRecording(void);
CameraControl_Status_t CameraControl_AppDriver_ToggleRecording(void);
CameraControl_Status_t CameraControl_AppDriver_SetVideoMode(void);
CameraControl_Status_t CameraControl_AppDriver_SetPhotoMode(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_CONTROL_APPDRIVER_H */
