#include "camera_board_app.h"

#include "camera_board_config.h"
#include "camera_board_output.h"
#include "camera_control_appdriver.h"
#include "camera_power_appdriver.h"
#include "status_led_appdriver.h"

static CameraBoard_AppState_t camera_board_state = CAMERA_BOARD_APP_STATE_UNINITIALIZED;
static CameraBoard_AppError_t camera_board_error = CAMERA_BOARD_APP_ERROR_NONE;
static uint32_t camera_board_state_entered_ms = 0U;
static bool camera_board_recording = false;

static bool CameraBoard_HasElapsed(uint32_t now_ms, uint32_t start_ms, uint32_t duration_ms)
{
    return ((now_ms - start_ms) >= duration_ms);
}

static const char *CameraBoard_GetStateOutputLine(CameraBoard_AppState_t state)
{
    switch (state)
    {
        case CAMERA_BOARD_APP_STATE_UNINITIALIZED:
            return "state: uninitialized";
        case CAMERA_BOARD_APP_STATE_BOOT_WAIT:
            return "state: boot_wait";
        case CAMERA_BOARD_APP_STATE_RECORDING:
            return "state: recording";
        case CAMERA_BOARD_APP_STATE_POST_STOP_WAIT:
            return "state: post_stop_wait";
        case CAMERA_BOARD_APP_STATE_COMPLETE:
            return "state: complete";
        case CAMERA_BOARD_APP_STATE_ERROR:
            return "state: error";
        default:
            return "state: unknown";
    }
}

static const char *CameraBoard_GetErrorOutputLine(CameraBoard_AppError_t error)
{
    switch (error)
    {
        case CAMERA_BOARD_APP_ERROR_NONE:
            return "error: none";
        case CAMERA_BOARD_APP_ERROR_INVALID_CONFIG:
            return "error: invalid app config";
        case CAMERA_BOARD_APP_ERROR_OUTPUT_INIT:
            return "error: output init failed";
        case CAMERA_BOARD_APP_ERROR_CAMERA_CONTROL_INIT:
            return "error: camera control init failed";
        case CAMERA_BOARD_APP_ERROR_CAMERA_POWER_ENABLE:
            return "error: camera power enable failed";
        case CAMERA_BOARD_APP_ERROR_START_RECORDING:
            return "error: start recording failed";
        case CAMERA_BOARD_APP_ERROR_STOP_RECORDING:
            return "error: stop recording failed";
        case CAMERA_BOARD_APP_ERROR_CAMERA_POWER_DISABLE:
            return "error: camera power disable failed";
        default:
            return "error: unknown";
    }
}

static void CameraBoard_OutputLine(const char *line)
{
    if (CameraBoard_Output_IsReady())
    {
        (void)CameraBoard_Output_WriteLine(line);
    }
}

static void CameraBoard_EnterState(CameraBoard_AppState_t state)
{
    camera_board_state = state;
    camera_board_state_entered_ms = HAL_GetTick();
    CameraBoard_OutputLine(CameraBoard_GetStateOutputLine(state));
}

static void CameraBoard_EnterError(CameraBoard_AppError_t error)
{
    camera_board_error = error;
    camera_board_recording = false;
    CameraBoard_EnterState(CAMERA_BOARD_APP_STATE_ERROR);
    StatusLed_AppDriver_SetState(STATUS_LED_STATE_ERROR);
    CameraBoard_OutputLine(CameraBoard_GetErrorOutputLine(error));
}

void CameraBoard_AppInit(const CameraBoard_AppConfig_t *config)
{
    camera_board_error = CAMERA_BOARD_APP_ERROR_NONE;
    camera_board_recording = false;

    StatusLed_AppDriver_Init();
    StatusLed_AppDriver_SetState(STATUS_LED_STATE_BOOTING);

    if ((config == NULL) || (config->camera_uart == NULL) || (config->output_uart == NULL))
    {
        CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_INVALID_CONFIG);
        return;
    }

    CameraBoard_OutputConfig_t output_config =
    {
        .uart = config->output_uart,
        .tx_timeout_ms = CAMERA_BOARD_OUTPUT_UART_TX_TIMEOUT_MS
    };

    if (CameraBoard_Output_Init(&output_config) != CAMERA_BOARD_OUTPUT_STATUS_OK)
    {
        CameraBoard_EnterError(CAMERA_BOARD_APP_ERROR_OUTPUT_INIT);
        return;
    }

    CameraBoard_OutputLine("camera-board: boot");

    CameraPower_AppDriver_Init();

    CameraControl_Config_t camera_control_config =
    {
        .huart = config->camera_uart,
        .tx_timeout_ms = CAMERA_BOARD_CAMERA_UART_TX_TIMEOUT_MS
    };

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

    CameraBoard_OutputLine("power: cameras enabled");
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
                CameraBoard_OutputLine("camera: start recording command sent");
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
                CameraBoard_OutputLine("camera: stop recording command sent");
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
                CameraBoard_OutputLine("power: cameras disabled");
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
