#include "camera_control_appdriver.h"

static UART_HandleTypeDef *camera_control_huart = NULL;
static uint32_t camera_control_tx_timeout_ms = CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS;
static bool camera_control_initialized = false;

static bool CameraControl_IsValidRunCamTxArgument(CameraControl_RunCamTxArgument_t argument)
{
    return (argument == CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_WIFI) ||
           (argument == CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_RECORD) ||
           (argument == CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_MODE);
}

CameraControl_Status_t CameraControl_AppDriver_Init(const CameraControl_Config_t *config)
{
    if ((config == NULL) || (config->huart == NULL))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    camera_control_huart = config->huart;
    camera_control_tx_timeout_ms = (config->tx_timeout_ms == 0U)
                                      ? CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS
                                      : config->tx_timeout_ms;

    camera_control_initialized = true;

    return CAMERA_CONTROL_STATUS_OK;
}

void CameraControl_AppDriver_DeInit(void)
{
    camera_control_huart = NULL;
    camera_control_initialized = false;
}

bool CameraControl_AppDriver_IsReady(void)
{
    return (camera_control_initialized && (camera_control_huart != NULL));
}

CameraControl_Status_t CameraControl_AppDriver_SendRaw(const uint8_t *bytes, uint8_t length)
{
    if (!CameraControl_AppDriver_IsReady())
    {
        return CAMERA_CONTROL_STATUS_NOT_INITIALIZED;
    }

    if ((bytes == NULL) || (length == 0U))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    const HAL_StatusTypeDef uart_status =
        HAL_UART_Transmit(camera_control_huart,
                          bytes,
                          length,
                          camera_control_tx_timeout_ms);

    if (uart_status == HAL_TIMEOUT)
    {
        return CAMERA_CONTROL_STATUS_UART_TIMEOUT;
    }

    if (uart_status != HAL_OK)
    {
        return CAMERA_CONTROL_STATUS_UART_ERROR;
    }

    return CAMERA_CONTROL_STATUS_OK;
}

uint8_t CameraControl_AppDriver_Crc8HighFirst(uint8_t crc, uint8_t data)
{
    crc ^= data;

    for (uint8_t i = 0U; i < 8U; i++)
    {
        if ((crc & 0x80U) != 0U)
        {
            crc = (uint8_t)((crc << 1U) ^ 0x31U);
        }
        else
        {
            crc = (uint8_t)(crc << 1U);
        }
    }

    return crc;
}

CameraControl_Status_t CameraControl_AppDriver_SendRunCamTxCommand(CameraControl_RunCamTxArgument_t argument)
{
    if (!CameraControl_IsValidRunCamTxArgument(argument))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    uint8_t packet[CAMERA_CONTROL_RUNCAM_TX_PACKET_LEN] =
    {
        CAMERA_CONTROL_RUNCAM_TX_HEADER,
        CAMERA_CONTROL_RUNCAM_TX_COMMAND_CAMERA_CONTROL,
        (uint8_t)argument,
        CAMERA_CONTROL_RUNCAM_TX_TAIL,
        CAMERA_CONTROL_RUNCAM_TX_TAIL
    };
    uint8_t crc = 0U;

    for (uint8_t i = 0U; i < 4U; i++)
    {
        crc = CameraControl_AppDriver_Crc8HighFirst(crc, packet[i]);
    }

    packet[3] = crc;

    return CameraControl_AppDriver_SendRaw(packet, (uint8_t)sizeof(packet));
}

CameraControl_Status_t CameraControl_AppDriver_StartRecording(void)
{
    return CameraControl_AppDriver_SendRunCamTxCommand(CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_RECORD);
}

CameraControl_Status_t CameraControl_AppDriver_StopRecording(void)
{
    return CameraControl_AppDriver_SendRunCamTxCommand(CAMERA_CONTROL_RUNCAM_TX_ARGUMENT_RECORD);
}
