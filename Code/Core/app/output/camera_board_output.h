#ifndef CAMERA_BOARD_OUTPUT_H
#define CAMERA_BOARD_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define CAMERA_BOARD_OUTPUT_DEFAULT_TX_TIMEOUT_MS 50U

typedef enum
{
    CAMERA_BOARD_OUTPUT_STATUS_OK = 0,
    CAMERA_BOARD_OUTPUT_STATUS_NOT_INITIALIZED,
    CAMERA_BOARD_OUTPUT_STATUS_INVALID_ARGUMENT,
    CAMERA_BOARD_OUTPUT_STATUS_UART_TIMEOUT,
    CAMERA_BOARD_OUTPUT_STATUS_UART_ERROR
} CameraBoard_OutputStatus_t;

typedef struct
{
    UART_HandleTypeDef *uart;
    uint32_t tx_timeout_ms;
} CameraBoard_OutputConfig_t;

CameraBoard_OutputStatus_t CameraBoard_Output_Init(const CameraBoard_OutputConfig_t *config);
void CameraBoard_Output_DeInit(void);
bool CameraBoard_Output_IsReady(void);
CameraBoard_OutputStatus_t CameraBoard_Output_Write(const char *text);
CameraBoard_OutputStatus_t CameraBoard_Output_WriteLine(const char *line);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_BOARD_OUTPUT_H */
