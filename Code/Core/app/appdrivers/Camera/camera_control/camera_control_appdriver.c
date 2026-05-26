#include "camera_control_appdriver.h"

#include <string.h>

static USART_TypeDef *camera_control_uart = NULL;
static uint32_t camera_control_tx_timeout_ms = CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS;
static CameraControl_Packet_t camera_control_packets[CAMERA_CONTROL_COMMAND_COUNT];
static bool camera_control_initialized = false;

static bool CameraControl_IsValidCommand(CameraControl_Command_t command)
{
    return ((uint32_t)command < (uint32_t)CAMERA_CONTROL_COMMAND_COUNT);
}

static uint32_t CameraControl_GetPeripheralClockHz(USART_TypeDef *uart)
{
#ifdef USART1
    if (uart == USART1)
    {
        return HAL_RCC_GetPCLK2Freq();
    }
#endif

#ifdef USART6
    if (uart == USART6)
    {
        return HAL_RCC_GetPCLK2Freq();
    }
#endif

    return HAL_RCC_GetPCLK1Freq();
}

static CameraControl_Status_t CameraControl_EnablePeripheralClock(USART_TypeDef *uart)
{
#ifdef UART4
    if (uart == UART4)
    {
        __HAL_RCC_UART4_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

#ifdef UART5
    if (uart == UART5)
    {
        __HAL_RCC_UART5_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

#ifdef USART1
    if (uart == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

#ifdef USART2
    if (uart == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

#ifdef USART3
    if (uart == USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

#ifdef USART6
    if (uart == USART6)
    {
        __HAL_RCC_USART6_CLK_ENABLE();
        return CAMERA_CONTROL_STATUS_OK;
    }
#endif

    return CAMERA_CONTROL_STATUS_UNSUPPORTED_UART;
}

static CameraControl_Status_t CameraControl_ConfigureTxOnlyUart(USART_TypeDef *uart, uint32_t baud_rate)
{
    if ((uart == NULL) || (baud_rate == 0U))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    const CameraControl_Status_t clock_status = CameraControl_EnablePeripheralClock(uart);

    if (clock_status != CAMERA_CONTROL_STATUS_OK)
    {
        return clock_status;
    }

    const uint32_t peripheral_clock_hz = CameraControl_GetPeripheralClockHz(uart);
    const uint32_t brr = (peripheral_clock_hz + (baud_rate / 2U)) / baud_rate;

    if (brr == 0U)
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    uart->CR1 = 0U;
    uart->CR2 = 0U;
    uart->CR3 = 0U;
    uart->BRR = brr;
    uart->CR1 = USART_CR1_TE | USART_CR1_UE;

    return CAMERA_CONTROL_STATUS_OK;
}

static CameraControl_Status_t CameraControl_WaitForFlag(uint32_t flag, uint32_t timeout_ms)
{
    const uint32_t start_ms = HAL_GetTick();

    while ((camera_control_uart->SR & flag) == 0U)
    {
        if ((HAL_GetTick() - start_ms) >= timeout_ms)
        {
            return CAMERA_CONTROL_STATUS_UART_TIMEOUT;
        }
    }

    return CAMERA_CONTROL_STATUS_OK;
}

CameraControl_Status_t CameraControl_AppDriver_Init(const CameraControl_Config_t *config)
{
    if ((config == NULL) || (config->uart == NULL))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    const uint32_t baud_rate = (config->baud_rate == 0U)
                                 ? CAMERA_CONTROL_DEFAULT_BAUD_RATE
                                 : config->baud_rate;

    if (config->configure_uart)
    {
        const CameraControl_Status_t configure_status =
            CameraControl_ConfigureTxOnlyUart(config->uart, baud_rate);

        if (configure_status != CAMERA_CONTROL_STATUS_OK)
        {
            return configure_status;
        }
    }

    camera_control_uart = config->uart;
    camera_control_tx_timeout_ms = (config->tx_timeout_ms == 0U)
                                      ? CAMERA_CONTROL_DEFAULT_TX_TIMEOUT_MS
                                      : config->tx_timeout_ms;

    for (uint32_t i = 0U; i < (uint32_t)CAMERA_CONTROL_COMMAND_COUNT; i++)
    {
        camera_control_packets[i].length = 0U;
    }

    camera_control_initialized = true;
    return CAMERA_CONTROL_STATUS_OK;
}

void CameraControl_AppDriver_DeInit(void)
{
    camera_control_uart = NULL;
    camera_control_initialized = false;

    for (uint32_t i = 0U; i < (uint32_t)CAMERA_CONTROL_COMMAND_COUNT; i++)
    {
        camera_control_packets[i].length = 0U;
    }
}

bool CameraControl_AppDriver_IsReady(void)
{
    return (camera_control_initialized && (camera_control_uart != NULL));
}

CameraControl_Status_t CameraControl_AppDriver_SetCommandPacket(CameraControl_Command_t command,
                                                                 const uint8_t *bytes,
                                                                 uint8_t length)
{
    if (!CameraControl_IsValidCommand(command) ||
        (bytes == NULL) ||
        (length == 0U) ||
        (length > CAMERA_CONTROL_MAX_PACKET_LEN))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    (void)memcpy(camera_control_packets[(uint32_t)command].bytes, bytes, length);
    camera_control_packets[(uint32_t)command].length = length;

    return CAMERA_CONTROL_STATUS_OK;
}

CameraControl_Status_t CameraControl_AppDriver_ClearCommandPacket(CameraControl_Command_t command)
{
    if (!CameraControl_IsValidCommand(command))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    camera_control_packets[(uint32_t)command].length = 0U;
    return CAMERA_CONTROL_STATUS_OK;
}

CameraControl_Status_t CameraControl_AppDriver_SendCommand(CameraControl_Command_t command)
{
    if (!CameraControl_AppDriver_IsReady())
    {
        return CAMERA_CONTROL_STATUS_NOT_INITIALIZED;
    }

    if (!CameraControl_IsValidCommand(command))
    {
        return CAMERA_CONTROL_STATUS_INVALID_ARGUMENT;
    }

    const CameraControl_Packet_t *packet = &camera_control_packets[(uint32_t)command];

    if (packet->length == 0U)
    {
        return CAMERA_CONTROL_STATUS_COMMAND_NOT_CONFIGURED;
    }

    return CameraControl_AppDriver_SendRaw(packet->bytes, packet->length);
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

    if ((camera_control_uart->CR1 & USART_CR1_UE) == 0U)
    {
        return CAMERA_CONTROL_STATUS_UART_NOT_ENABLED;
    }

    for (uint8_t i = 0U; i < length; i++)
    {
        const CameraControl_Status_t txe_status =
            CameraControl_WaitForFlag(USART_SR_TXE, camera_control_tx_timeout_ms);

        if (txe_status != CAMERA_CONTROL_STATUS_OK)
        {
            return txe_status;
        }

        camera_control_uart->DR = bytes[i];
    }

    const CameraControl_Status_t complete_status =
        CameraControl_WaitForFlag(USART_SR_TC, camera_control_tx_timeout_ms);

    if (complete_status != CAMERA_CONTROL_STATUS_OK)
    {
        return complete_status;
    }

    return CAMERA_CONTROL_STATUS_OK;
}

CameraControl_Status_t CameraControl_AppDriver_StartRecording(void)
{
    return CameraControl_AppDriver_SendCommand(CAMERA_CONTROL_COMMAND_START_RECORDING);
}

CameraControl_Status_t CameraControl_AppDriver_StopRecording(void)
{
    return CameraControl_AppDriver_SendCommand(CAMERA_CONTROL_COMMAND_STOP_RECORDING);
}

CameraControl_Status_t CameraControl_AppDriver_ToggleRecording(void)
{
    return CameraControl_AppDriver_SendCommand(CAMERA_CONTROL_COMMAND_TOGGLE_RECORDING);
}

CameraControl_Status_t CameraControl_AppDriver_SetVideoMode(void)
{
    return CameraControl_AppDriver_SendCommand(CAMERA_CONTROL_COMMAND_SET_VIDEO_MODE);
}

CameraControl_Status_t CameraControl_AppDriver_SetPhotoMode(void)
{
    return CameraControl_AppDriver_SendCommand(CAMERA_CONTROL_COMMAND_SET_PHOTO_MODE);
}
