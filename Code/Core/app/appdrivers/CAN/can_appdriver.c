#include "can_appdriver.h"

#include <string.h>

static CAN_HandleTypeDef *can_appdriver_hcan = NULL;
static CAN_AppDriver_Config_t can_appdriver_config;
static CAN_AppDriver_Message_t can_appdriver_rx_queue[CAN_APPDRIVER_RX_QUEUE_LEN];
static volatile uint8_t can_appdriver_rx_head = 0U;
static volatile uint8_t can_appdriver_rx_tail = 0U;
static volatile uint8_t can_appdriver_rx_count = 0U;
static uint32_t can_appdriver_dropped_rx_count = 0U;
static uint32_t can_appdriver_last_hal_error = 0U;
static bool can_appdriver_initialized = false;
static bool can_appdriver_started = false;

static bool CAN_AppDriver_IsValidStdId(uint32_t std_id)
{
    return (std_id <= 0x7FFU);
}

static uint32_t CAN_AppDriver_GetConfiguredFilterBank(uint32_t filter_bank)
{
    return (filter_bank > 27U) ? CAN_APPDRIVER_DEFAULT_FILTER_BANK : filter_bank;
}

static uint32_t CAN_AppDriver_GetConfiguredSlaveStartBank(uint32_t slave_start_bank)
{
    return (slave_start_bank > 27U) ? CAN_APPDRIVER_DEFAULT_SLAVE_START_FILTER_BANK : slave_start_bank;
}

static void CAN_AppDriver_SaveHalError(void)
{
    if (can_appdriver_hcan != NULL)
    {
        can_appdriver_last_hal_error = HAL_CAN_GetError(can_appdriver_hcan);
    }
}

static void CAN_AppDriver_ClearRxQueue(void)
{
    can_appdriver_rx_head = 0U;
    can_appdriver_rx_tail = 0U;
    can_appdriver_rx_count = 0U;
}

static bool CAN_AppDriver_PushRxMessage(const CAN_AppDriver_Message_t *message)
{
    if (can_appdriver_rx_count >= CAN_APPDRIVER_RX_QUEUE_LEN)
    {
        can_appdriver_dropped_rx_count++;
        return false;
    }

    can_appdriver_rx_queue[can_appdriver_rx_head] = *message;
    can_appdriver_rx_head = (uint8_t)((can_appdriver_rx_head + 1U) % CAN_APPDRIVER_RX_QUEUE_LEN);
    can_appdriver_rx_count++;

    return true;
}

static CAN_AppDriver_Status_t CAN_AppDriver_ConfigureFilter(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = CAN_AppDriver_GetConfiguredFilterBank(can_appdriver_config.filter_bank);
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank =
        CAN_AppDriver_GetConfiguredSlaveStartBank(can_appdriver_config.slave_start_filter_bank);

    if (can_appdriver_config.accept_all_rx)
    {
        filter.FilterIdHigh = 0x0000U;
        filter.FilterIdLow = 0x0000U;
        filter.FilterMaskIdHigh = 0x0000U;
        filter.FilterMaskIdLow = 0x0000U;
    }
    else
    {
        filter.FilterIdHigh = (uint16_t)(can_appdriver_config.command_std_id << 5U);
        filter.FilterIdLow = 0x0000U;
        filter.FilterMaskIdHigh = (uint16_t)(0x7FFU << 5U);
        filter.FilterMaskIdLow = 0x0000U;
    }

    if (HAL_CAN_ConfigFilter(can_appdriver_hcan, &filter) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    return CAN_APPDRIVER_STATUS_OK;
}

static CAN_AppDriver_Status_t CAN_AppDriver_ReadHardwareRxFifo(void)
{
    CAN_RxHeaderTypeDef rx_header = {0};
    CAN_AppDriver_Message_t message = {0};
    CAN_AppDriver_Status_t status = CAN_APPDRIVER_STATUS_OK;

    while (HAL_CAN_GetRxFifoFillLevel(can_appdriver_hcan, CAN_RX_FIFO0) > 0U)
    {
        if (HAL_CAN_GetRxMessage(can_appdriver_hcan,
                                 CAN_RX_FIFO0,
                                 &rx_header,
                                 message.data) != HAL_OK)
        {
            CAN_AppDriver_SaveHalError();
            return CAN_APPDRIVER_STATUS_HAL_ERROR;
        }

        message.is_extended_id = (rx_header.IDE == CAN_ID_EXT);
        message.is_remote_frame = (rx_header.RTR == CAN_RTR_REMOTE);
        message.id = message.is_extended_id ? rx_header.ExtId : rx_header.StdId;
        message.length = (rx_header.DLC > CAN_APPDRIVER_MAX_DATA_LEN)
                           ? CAN_APPDRIVER_MAX_DATA_LEN
                           : (uint8_t)rx_header.DLC;

        if (!CAN_AppDriver_PushRxMessage(&message))
        {
            status = CAN_APPDRIVER_STATUS_RX_QUEUE_FULL;
        }
    }

    return status;
}

void CAN_AppDriver_GetDefaultConfig(CAN_HandleTypeDef *hcan, CAN_AppDriver_Config_t *config)
{
    if (config == NULL)
    {
        return;
    }

    config->hcan = hcan;
    config->command_std_id = CAN_APPDRIVER_DEFAULT_COMMAND_STD_ID;
    config->status_std_id = CAN_APPDRIVER_DEFAULT_STATUS_STD_ID;
    config->ack_std_id = CAN_APPDRIVER_DEFAULT_ACK_STD_ID;
    config->heartbeat_std_id = CAN_APPDRIVER_DEFAULT_HEARTBEAT_STD_ID;
    config->filter_bank = CAN_APPDRIVER_DEFAULT_FILTER_BANK;
    config->slave_start_filter_bank = CAN_APPDRIVER_DEFAULT_SLAVE_START_FILTER_BANK;
    config->accept_all_rx = false;
}

CAN_AppDriver_Status_t CAN_AppDriver_Init(const CAN_AppDriver_Config_t *config)
{
    if ((config == NULL) || (config->hcan == NULL))
    {
        return CAN_APPDRIVER_STATUS_INVALID_ARGUMENT;
    }

    if (!CAN_AppDriver_IsValidStdId(config->command_std_id) ||
        !CAN_AppDriver_IsValidStdId(config->status_std_id) ||
        !CAN_AppDriver_IsValidStdId(config->ack_std_id) ||
        !CAN_AppDriver_IsValidStdId(config->heartbeat_std_id))
    {
        return CAN_APPDRIVER_STATUS_INVALID_ARGUMENT;
    }

    can_appdriver_hcan = config->hcan;
    can_appdriver_config = *config;
    can_appdriver_last_hal_error = 0U;
    can_appdriver_dropped_rx_count = 0U;
    CAN_AppDriver_ClearRxQueue();

#ifdef CAN2
    if (can_appdriver_hcan->Instance == CAN2)
    {
        __HAL_RCC_CAN1_CLK_ENABLE();
    }
#endif

    const CAN_AppDriver_Status_t filter_status = CAN_AppDriver_ConfigureFilter();

    if (filter_status != CAN_APPDRIVER_STATUS_OK)
    {
        can_appdriver_initialized = false;
        return filter_status;
    }

    can_appdriver_initialized = true;
    can_appdriver_started = false;

    return CAN_APPDRIVER_STATUS_OK;
}

CAN_AppDriver_Status_t CAN_AppDriver_Start(void)
{
    if (!can_appdriver_initialized || (can_appdriver_hcan == NULL))
    {
        return CAN_APPDRIVER_STATUS_NOT_INITIALIZED;
    }

    if (HAL_CAN_Start(can_appdriver_hcan) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    if (HAL_CAN_ActivateNotification(can_appdriver_hcan,
                                      CAN_IT_RX_FIFO0_MSG_PENDING |
                                      CAN_IT_ERROR |
                                      CAN_IT_BUSOFF) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    can_appdriver_started = true;
    return CAN_APPDRIVER_STATUS_OK;
}

CAN_AppDriver_Status_t CAN_AppDriver_Stop(void)
{
    if (!can_appdriver_initialized || (can_appdriver_hcan == NULL))
    {
        return CAN_APPDRIVER_STATUS_NOT_INITIALIZED;
    }

    if (HAL_CAN_DeactivateNotification(can_appdriver_hcan,
                                        CAN_IT_RX_FIFO0_MSG_PENDING |
                                        CAN_IT_ERROR |
                                        CAN_IT_BUSOFF) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    if (HAL_CAN_Stop(can_appdriver_hcan) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    can_appdriver_started = false;
    return CAN_APPDRIVER_STATUS_OK;
}

bool CAN_AppDriver_IsStarted(void)
{
    return can_appdriver_started;
}

CAN_AppDriver_Status_t CAN_AppDriver_PollRx(void)
{
    if (!can_appdriver_started || (can_appdriver_hcan == NULL))
    {
        return CAN_APPDRIVER_STATUS_NOT_INITIALIZED;
    }

    return CAN_AppDriver_ReadHardwareRxFifo();
}

CAN_AppDriver_Status_t CAN_AppDriver_Read(CAN_AppDriver_Message_t *message)
{
    if (message == NULL)
    {
        return CAN_APPDRIVER_STATUS_INVALID_ARGUMENT;
    }

    if (can_appdriver_rx_count == 0U)
    {
        return CAN_APPDRIVER_STATUS_RX_QUEUE_EMPTY;
    }

    *message = can_appdriver_rx_queue[can_appdriver_rx_tail];
    can_appdriver_rx_tail = (uint8_t)((can_appdriver_rx_tail + 1U) % CAN_APPDRIVER_RX_QUEUE_LEN);
    can_appdriver_rx_count--;

    return CAN_APPDRIVER_STATUS_OK;
}

uint8_t CAN_AppDriver_GetPendingRxCount(void)
{
    return can_appdriver_rx_count;
}

uint32_t CAN_AppDriver_GetDroppedRxCount(void)
{
    return can_appdriver_dropped_rx_count;
}

uint32_t CAN_AppDriver_GetLastHalError(void)
{
    return can_appdriver_last_hal_error;
}

CAN_AppDriver_Status_t CAN_AppDriver_Send(uint32_t std_id, const uint8_t *data, uint8_t length)
{
    CAN_AppDriver_Message_t message = {0};

    if ((length > CAN_APPDRIVER_MAX_DATA_LEN) || ((data == NULL) && (length > 0U)))
    {
        return CAN_APPDRIVER_STATUS_INVALID_ARGUMENT;
    }

    message.id = std_id;
    message.is_extended_id = false;
    message.is_remote_frame = false;
    message.length = length;

    if ((data != NULL) && (length > 0U))
    {
        (void)memcpy(message.data, data, length);
    }

    return CAN_AppDriver_SendMessage(&message);
}

CAN_AppDriver_Status_t CAN_AppDriver_SendMessage(const CAN_AppDriver_Message_t *message)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox = 0U;

    if (!can_appdriver_started || (can_appdriver_hcan == NULL))
    {
        return CAN_APPDRIVER_STATUS_NOT_INITIALIZED;
    }

    if ((message == NULL) ||
        (message->length > CAN_APPDRIVER_MAX_DATA_LEN) ||
        (!message->is_extended_id && !CAN_AppDriver_IsValidStdId(message->id)) ||
        (message->is_extended_id && (message->id > 0x1FFFFFFFU)))
    {
        return CAN_APPDRIVER_STATUS_INVALID_ARGUMENT;
    }

    if (HAL_CAN_GetTxMailboxesFreeLevel(can_appdriver_hcan) == 0U)
    {
        return CAN_APPDRIVER_STATUS_TX_MAILBOX_FULL;
    }

    tx_header.IDE = message->is_extended_id ? CAN_ID_EXT : CAN_ID_STD;
    tx_header.RTR = message->is_remote_frame ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = message->length;
    tx_header.TransmitGlobalTime = DISABLE;

    if (message->is_extended_id)
    {
        tx_header.ExtId = message->id;
    }
    else
    {
        tx_header.StdId = message->id;
    }

    if (HAL_CAN_AddTxMessage(can_appdriver_hcan,
                             &tx_header,
                             (uint8_t *)message->data,
                             &tx_mailbox) != HAL_OK)
    {
        CAN_AppDriver_SaveHalError();
        return CAN_APPDRIVER_STATUS_HAL_ERROR;
    }

    return CAN_APPDRIVER_STATUS_OK;
}

CAN_AppDriver_Status_t CAN_AppDriver_SendAck(uint8_t command_id, uint8_t result_code)
{
    const uint8_t payload[] = {command_id, result_code};
    return CAN_AppDriver_Send(can_appdriver_config.ack_std_id,
                              payload,
                              (uint8_t)sizeof(payload));
}

CAN_AppDriver_Status_t CAN_AppDriver_SendHeartbeat(uint8_t board_state, uint8_t error_flags)
{
    const uint8_t payload[] = {board_state, error_flags};
    return CAN_AppDriver_Send(can_appdriver_config.heartbeat_std_id,
                              payload,
                              (uint8_t)sizeof(payload));
}

CAN_AppDriver_Status_t CAN_AppDriver_SendCameraStatus(uint8_t power_mask,
                                                       uint8_t recording_mask,
                                                       uint8_t error_flags)
{
    const uint8_t payload[] = {power_mask, recording_mask, error_flags};
    return CAN_AppDriver_Send(can_appdriver_config.status_std_id,
                              payload,
                              (uint8_t)sizeof(payload));
}

void CAN_AppDriver_OnRxFifo0MsgPending(CAN_HandleTypeDef *hcan)
{
    if ((hcan == NULL) || (hcan != can_appdriver_hcan))
    {
        return;
    }

    (void)CAN_AppDriver_ReadHardwareRxFifo();
}
