#ifndef CAMERA_BOARD_APP_H
#define CAMERA_BOARD_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    CAMERA_BOARD_APP_STATE_UNINITIALIZED = 0,
    CAMERA_BOARD_APP_STATE_BOOT_WAIT,
    CAMERA_BOARD_APP_STATE_RECORDING,
    CAMERA_BOARD_APP_STATE_POST_STOP_WAIT,
    CAMERA_BOARD_APP_STATE_COMPLETE,
    CAMERA_BOARD_APP_STATE_ERROR
} CameraBoard_AppState_t;

typedef enum
{
    CAMERA_BOARD_APP_ERROR_NONE = 0,
    CAMERA_BOARD_APP_ERROR_CAMERA_CONTROL_INIT,
    CAMERA_BOARD_APP_ERROR_CAMERA_POWER_ENABLE,
    CAMERA_BOARD_APP_ERROR_START_RECORDING,
    CAMERA_BOARD_APP_ERROR_STOP_RECORDING,
    CAMERA_BOARD_APP_ERROR_CAMERA_POWER_DISABLE
} CameraBoard_AppError_t;

void CameraBoard_AppInit(void);
void CameraBoard_AppRun(void);
CameraBoard_AppState_t CameraBoard_AppGetState(void);
CameraBoard_AppError_t CameraBoard_AppGetError(void);
bool CameraBoard_AppIsRecording(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_BOARD_APP_H */
