#include "camera_board_app.h"

#include "camera_board_config.h"
#include "camera_control_appdriver.h"
#include "camera_power_appdriver.h"
#include "main.h"
#include "status_led_appdriver.h"

static CameraBoard_AppState_t camera_board_state = CAMERA_BOARD_APP_STATE_UNINITIALIZED;
static CameraBoard_AppError_t camera_board_error = CAMERA_BOARD_APP_ERROR_NONE;
static uint32_t camera_board_state_entered_ms = 0U;
static bool camera_board_recording = false;

static bool CameraBoard_HasElapsed(uint32_t now_ms, uint32_t start_ms, uint32_t duration_ms)
{
    return ((now_ms - start_ms) >= duration_ms);
}

static void CameraBoard_EnterState(CameraBoard_AppState_t state)
{
    camera_board_state = state;
    camera_board_state_entered_ms = HAL_GetTick();
}

static void CameraBoard_EnterError(CameraBoard_AppError_t error)
{
    camera_board_error = error;
    camera_board_recording = false;
    CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_ERROR);
    StatusLed_AppDriver_SetState(STATUS_LED_STATE_ERROR);
}

void CameraBoard_AppInit(void)
{
    CameraControl_Config_t camera_control_config =
    {
        .uart = UART4,
        .baud_rate = CAMERA_BOARD_CAMERA_UART_BAUD_RATE,
        .tx_timeout_ms = CAMERA_BOARD_CAMERA_UART_TX_TIMEOUT_MS,
        .configure_uart = true
    };

    camera_board_error = CAMERA_BOARD_APP_ERROR_NONE;
    camera_board_recording = false;

    StatusLed_AppDriver_Init();
    StatusLed_AppDriver_SetState(STATUS_LED_STATE_BOOTING);

    CameraPower_AppDriver_Init();

    if (CameraControl_AppDriver_Init(&camera_control_config) != CAMERA_CONTROL_STATUS_OK)
    {
        CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_CAMERA_CONTROL_INIT);
        return;
    }

    if (CameraPower_AppDriver_Enable(CAMERA_POWER_ID_ALL) != CAMERA_POWER_STATUS_OK)
    {
        CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_CAMERA_POWER_ENABLE);
        return;
    }

    CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_BOOT_WAIT);
}

void CameraBoard_AppRun(void)
{
    const uint32_t now_ms = HAL_GetTick();

    StatusLed_AppDriver_Task(now_ms);

    switch (camera_board_state)
    {
        case CAMERA_BOARD_APP_STATE_BOOT_WAIT:
            if (CameraBoard_HasElapsed(now_ms,
                                       camera_board_state_entered_ms,
                                       CAMERA_BOARD_CAMERA_BOOT_DELAY_MS))
            {
                if (CameraControl_AppDriver_StartRecording() != CAMERA_CONTROL_STATUS_OK)
                {
                    CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_START_RECORDING);
                    return;
                }

                camera_board_recording = true;
                StatusLed_AppDriver_SetState(STATUS_LED_STATE_RECORDING);
                CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_RECORDING);
            }
            break;

        case CAMERA_BOARD_APP_STATE_RECORDING:
            if (CameraBoard_HasElapsed(now_ms,
                                       camera_board_state_entered_ms,
                                       CAMERA_BOARD_RECORD_DURATION_MS))
            {
                if (CameraControl_AppDriver_StopRecording() != CAMERA_CONTROL_STATUS_OK)
                {
                    CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_STOP_RECORDING);
                    return;
                }

                camera_board_recording = false;
                StatusLed_AppDriver_SetState(STATUS_LED_STATE_IDLE);
                CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_POST_STOP_WAIT);
            }
            break;

        case CAMERA_BOARD_APP_STATE_POST_STOP_WAIT:
            if (CameraBoard_HasElapsed(now_ms,
                                       camera_board_state_entered_ms,
                                       CAMERA_BOARD_POST_STOP_POWER_OFF_DELAY_MS))
            {
#if CAMERA_BOARD_POWER_OFF_AFTER_RECORDING
                if (CameraPower_AppDriver_Disable(CAMERA_POWER_ID_ALL) != CAMERA_POWER_STATUS_OK)
                {
                    CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_CAMERA_POWER_DISABLE);
                    return;
                }
#endif
                CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_COMPLETE);
            }
            break;

        case CAMERA_BOARD_APP_STATE_COMPLETE:
        case CAMERA_BOARD_APP_STATE_ERROR:
        case CAMERA_BOARD_APP_STATE_UNINITIALIZED:
        default:
            break;
    }
}

CameraBoard_AppState_t CameraBoard_AppGetState(void)
{
    return camera_board_state;
}

CameraBoard_AppError_t CameraBoard_AppGetError(void)
{
    return camera_board_error;
}

bool CameraBoard_AppIsRecording(void)
{
    return camera_board_recording;
}
