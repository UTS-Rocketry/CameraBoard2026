#ifndef CAN_APPDRIVER_H
#define CAN_APPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define CAN_APPDRIVER_MAX_DATA_LEN 8U
#define CAN_APPDRIVER_RX_QUEUE_LEN 8U

#define CAN_APPDRIVER_DEFAULT_COMMAND_STD_ID   0x250U
#define CAN_APPDRIVER_DEFAULT_STATUS_STD_ID    0x251U
#define CAN_APPDRIVER_DEFAULT_ACK_STD_ID       0x252U
#define CAN_APPDRIVER_DEFAULT_HEARTBEAT_STD_ID 0x253U

#define CAN_APPDRIVER_DEFAULT_FILTER_BANK 14U
#define CAN_APPDRIVER_DEFAULT_SLAVE_START_FILTER_BANK 14U

typedef enum
{
    CAN_APPDRIVER_STATUS_OK = 0,
    CAN_APPDRIVER_STATUS_NOT_INITIALIZED,
    CAN_APPDRIVER_STATUS_INVALID_ARGUMENT,
    CAN_APPDRIVER_STATUS_HAL_ERROR,
    CAN_APPDRIVER_STATUS_TX_MAILBOX_FULL,
    CAN_APPDRIVER_STATUS_RX_QUEUE_EMPTY,
    CAN_APPDRIVER_STATUS_RX_QUEUE_FULL
} CAN_AppDriver_Status_t;

typedef struct
{
    CAN_HandleTypeDef *hcan;
    uint32_t command_std_id;
    uint32_t status_std_id;
    uint32_t ack_std_id;
    uint32_t heartbeat_std_id;
    uint32_t filter_bank;
    uint32_t slave_start_filter_bank;
    bool accept_all_rx;
} CAN_AppDriver_Config_t;

typedef struct
{
    uint32_t id;
    bool is_extended_id;
    bool is_remote_frame;
    uint8_t length;
    uint8_t data[CAN_APPDRIVER_MAX_DATA_LEN];
} CAN_AppDriver_Message_t;

void CAN_AppDriver_GetDefaultConfig(CAN_HandleTypeDef *hcan, CAN_AppDriver_Config_t *config);
CAN_AppDriver_Status_t CAN_AppDriver_Init(const CAN_AppDriver_Config_t *config);
CAN_AppDriver_Status_t CAN_AppDriver_Start(void);
CAN_AppDriver_Status_t CAN_AppDriver_Stop(void);
bool CAN_AppDriver_IsStarted(void);
CAN_AppDriver_Status_t CAN_AppDriver_PollRx(void);
CAN_AppDriver_Status_t CAN_AppDriver_Read(CAN_AppDriver_Message_t *message);
uint8_t CAN_AppDriver_GetPendingRxCount(void);
uint32_t CAN_AppDriver_GetDroppedRxCount(void);
uint32_t CAN_AppDriver_GetLastHalError(void);
CAN_AppDriver_Status_t CAN_AppDriver_Send(uint32_t std_id, const uint8_t *data, uint8_t length);
CAN_AppDriver_Status_t CAN_AppDriver_SendMessage(const CAN_AppDriver_Message_t *message);
CAN_AppDriver_Status_t CAN_AppDriver_SendAck(uint8_t command_id, uint8_t result_code);
CAN_AppDriver_Status_t CAN_AppDriver_SendHeartbeat(uint8_t board_state, uint8_t error_flags);
CAN_AppDriver_Status_t CAN_AppDriver_SendCameraStatus(uint8_t power_mask,
                                                       uint8_t recording_mask,
                                                       uint8_t error_flags);
void CAN_AppDriver_OnRxFifo0MsgPending(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APPDRIVER_H */
