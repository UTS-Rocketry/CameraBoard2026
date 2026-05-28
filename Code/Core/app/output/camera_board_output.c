#include "camera_board_output.h"

#include <stddef.h>
#include <string.h>

static UART_HandleTypeDef *camera_board_output_uart = NULL;
static uint32_t camera_board_output_tx_timeout_ms = CAMERA_BOARD_OUTPUT_DEFAULT_TX_TIMEOUT_MS;
static bool camera_board_output_initialized = false;

static CameraBoard_OutputStatus_t CameraBoard_Output_Transmit(const char *text, size_t length)
{
    size_t bytes_sent = 0U;

    while (bytes_sent < length)
    {
        const size_t bytes_remaining = length - bytes_sent;
        const uint16_t chunk_length = (bytes_remaining > UINT16_MAX)
                                        ? UINT16_MAX
                                        : (uint16_t)bytes_remaining;
        const HAL_StatusTypeDef uart_status =
            HAL_UART_Transmit(camera_board_output_uart,
                              (const uint8_t *)&text[bytes_sent],
                              chunk_length,
                              camera_board_output_tx_timeout_ms);

        if (uart_status == HAL_TIMEOUT)
        {
            return CAMERA_BOARD_OUTPUT_STATUS_UART_TIMEOUT;
        }

        if (uart_status != HAL_OK)
        {
            return CAMERA_BOARD_OUTPUT_STATUS_UART_ERROR;
        }

        bytes_sent += chunk_length;
    }

    return CAMERA_BOARD_OUTPUT_STATUS_OK;
}

CameraBoard_OutputStatus_t CameraBoard_Output_Init(const CameraBoard_OutputConfig_t *config)
{
    if ((config == NULL) || (config->uart == NULL))
    {
        return CAMERA_BOARD_OUTPUT_STATUS_INVALID_ARGUMENT;
    }

    camera_board_output_uart = config->uart;
    camera_board_output_tx_timeout_ms = (config->tx_timeout_ms == 0U)
                                          ? CAMERA_BOARD_OUTPUT_DEFAULT_TX_TIMEOUT_MS
                                          : config->tx_timeout_ms;
    camera_board_output_initialized = true;

    return CAMERA_BOARD_OUTPUT_STATUS_OK;
}

void CameraBoard_Output_DeInit(void)
{
    camera_board_output_uart = NULL;
    camera_board_output_initialized = false;
}

bool CameraBoard_Output_IsReady(void)
{
    return (camera_board_output_initialized && (camera_board_output_uart != NULL));
}

CameraBoard_OutputStatus_t CameraBoard_Output_Write(const char *text)
{
    if (!CameraBoard_Output_IsReady())
    {
        return CAMERA_BOARD_OUTPUT_STATUS_NOT_INITIALIZED;
    }

    if (text == NULL)
    {
        return CAMERA_BOARD_OUTPUT_STATUS_INVALID_ARGUMENT;
    }

    return CameraBoard_Output_Transmit(text, strlen(text));
}

CameraBoard_OutputStatus_t CameraBoard_Output_WriteLine(const char *line)
{
    CameraBoard_OutputStatus_t status = CameraBoard_Output_Write(line);

    if (status != CAMERA_BOARD_OUTPUT_STATUS_OK)
    {
        return status;
    }

    return CameraBoard_Output_Write("\r\n");
}
