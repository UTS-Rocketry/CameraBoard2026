#ifndef CAMERA_CONTROL_APPDRIVER_H
#define CAMERA_CONTROL_APPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS 50U
#define CAMERA_CONTROL_RUNCAM_TX_HEADER 0x55U
#define CAMERA_CONTROL_RUNCAM_TX_TAIL 0xAAU
#define CAMERA_CONTROL_RUNCAM_TX_PACKET_LEN 5U

typedef enum
{
    CAMERA_CONTROL_STATUS_OK = 0,
    CAMERA_CONTROL_STATUS_NOT_INITIALIZED,
    CAMERA_CONTROL_STATUS_INVALID_ARGUMENT,
    CAMERA_CONTROL_STATUS_UART_TIMEOUT,
    CAMERA_CONTROL_STATUS_UART_ERROR
} CameraControl_Status_t;

typedef enum
{
    CAMERA_CONTROL_RUNCAM_TX_COMMAND_CAMERA_CONTROL = 0x01
} CameraControl_RunCamTxCommand_t;

typedef enum
{
    CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_WIFI = 0x01,
    CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_RECORD = 0x02,
    CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_MODE = 0x03
} CameraControl_RunCamTxArgument_t;

typedef struct
{
    UART_HandleTypeDef *huart;
    uint32_t tx_timeout_ms;
} CameraControl_Config_t;

CameraControl_Status_t CameraControl_AppDriver_Init(const CameraControl_Config_t *config);
void CameraControl_AppDriver_DeInit(void);
bool CameraControl_AppDriver_IsReady(void);
CameraControl_Status_t CameraControl_AppDriver_SendRaw(const uint8_t *bytes, uint8_t length);
uint8_t CameraControl_AppDriver_Crc8HighFirst(uint8_t crc, uint8_t data);
CameraControl_Status_t CameraControl_AppDriver_SendRunCamTxCommand(CameraControl_RunCamTxArgument_t argument);
CameraControl_Status_t CameraControl_AppDriver_StartRecording(void);
CameraControl_Status_t CameraControl_AppDriver_StopRecording(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_CONTROL_APPDRIVER_H */
