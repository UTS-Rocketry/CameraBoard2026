#ifndef CAMERA_POWER_APPDRIVER_H
#define CAMERA_POWER_APPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    CAMERA_POWER_STATUS_OK = 0,
    CAMERA_POWER_STATUS_INVALID_CAMERA,
    CAMERA_POWER_STATUS_NOT_INITIALIZED
} CameraPower_Status_t;

typedef enum
{
    CAMERA_POWER_ID_1 = 0,
    CAMERA_POWER_ID_LEFT = CAMERA_POWER_ID_1,
    CAMERA_POWER_ID_2,
    CAMERA_POWER_ID_RIGHT = CAMERA_POWER_ID_2,
    CAMERA_POWER_ID_3,
    CAMERA_POWER_ID_CENTER = CAMERA_POWER_ID_3,
    CAMERA_POWER_ID_ALL = 0xFF
} CameraPower_Id_t;

#define CAMERA_POWER_MASK_1      (1U << 0)
#define CAMERA_POWER_MASK_LEFT   CAMERA_POWER_MASK_1
#define CAMERA_POWER_MASK_2      (1U << 1)
#define CAMERA_POWER_MASK_RIGHT  CAMERA_POWER_MASK_2
#define CAMERA_POWER_MASK_3      (1U << 2)
#define CAMERA_POWER_MASK_CENTER CAMERA_POWER_MASK_3
#define CAMERA_POWER_MASK_ALL    (CAMERA_POWER_MASK_1 | CAMERA_POWER_MASK_2 | CAMERA_POWER_MASK_3)

void CameraPower_AppDriver_Init(void);
CameraPower_Status_t CameraPower_AppDriver_Enable(CameraPower_Id_t camera);
CameraPower_Status_t CameraPower_AppDriver_Disable(CameraPower_Id_t camera);
CameraPower_Status_t CameraPower_AppDriver_Set(CameraPower_Id_t camera, bool enabled);
CameraPower_Status_t CameraPower_AppDriver_Toggle(CameraPower_Id_t camera);
bool CameraPower_AppDriver_IsEnabled(CameraPower_Id_t camera);
uint8_t CameraPower_AppDriver_GetEnabledMask(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_POWER_APPDRIVER_H */
