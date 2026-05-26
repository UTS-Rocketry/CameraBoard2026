#include "camera_power_appdriver.h"

#include "main.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t mask;
} CameraPower_PinMap_t;

static const CameraPower_PinMap_t camera_power_pin_map[] =
{
    {CAM_L_PWN_EN_GPIO_Port, CAM_L_PWN_EN_Pin, CAMERA_POWER_MASK_1},
    {CAM_R_PWN_EN_GPIO_Port, CAM_R_PWN_EN_Pin, CAMERA_POWER_MASK_2},
    {CAM_C_PWN_EN_GPIO_Port, CAM_C_PWN_EN_Pin, CAMERA_POWER_MASK_3}
};

static bool camera_power_initialized = false;
static uint8_t camera_power_enabled_mask = 0U;

static bool CameraPower_IsValidSingleCamera(CameraPower_Id_t camera)
{
    return (camera == CAMERA_POWER_ID_1) ||
           (camera == CAMERA_POWER_ID_2) ||
           (camera == CAMERA_POWER_ID_3);
}

static CameraPower_Status_t CameraPower_SetSingle(CameraPower_Id_t camera, bool enabled)
{
    if (!CameraPower_IsValidSingleCamera(camera))
    {
        return CAMERA_POWER_STATUS_INVALID_CAMERA;
    }

    const CameraPower_PinMap_t *pin_map = &camera_power_pin_map[(uint8_t)camera];
    HAL_GPIO_WritePin(pin_map->port,
                      pin_map->pin,
                      enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);

    if (enabled)
    {
        camera_power_enabled_mask |= pin_map->mask;
    }
    else
    {
        camera_power_enabled_mask &= (uint8_t)(~pin_map->mask);
    }

    return CAMERA_POWER_STATUS_OK;
}

void CameraPower_AppDriver_Init(void)
{
    (void)CameraPower_AppDriver_Disable(CAMERA_POWER_ID_ALL);
    camera_power_initialized = true;
}

CameraPower_Status_t CameraPower_AppDriver_Enable(CameraPower_Id_t camera)
{
    return CameraPower_AppDriver_Set(camera, true);
}

CameraPower_Status_t CameraPower_AppDriver_Disable(CameraPower_Id_t camera)
{
    return CameraPower_AppDriver_Set(camera, false);
}

CameraPower_Status_t CameraPower_AppDriver_Set(CameraPower_Id_t camera, bool enabled)
{
    if (!camera_power_initialized && (camera != CAMERA_POWER_ID_ALL))
    {
        return CAMERA_POWER_STATUS_NOT_INITIALIZED;
    }

    if (camera == CAMERA_POWER_ID_ALL)
    {
        for (uint8_t i = 0U; i < (uint8_t)(sizeof(camera_power_pin_map) / sizeof(camera_power_pin_map[0])); i++)
        {
            (void)CameraPower_SetSingle((CameraPower_Id_t)i, enabled);
        }

        return CAMERA_POWER_STATUS_OK;
    }

    return CameraPower_SetSingle(camera, enabled);
}

CameraPower_Status_t CameraPower_AppDriver_Toggle(CameraPower_Id_t camera)
{
    if (!camera_power_initialized)
    {
        return CAMERA_POWER_STATUS_NOT_INITIALIZED;
    }

    if (camera == CAMERA_POWER_ID_ALL)
    {
        const bool all_enabled = ((camera_power_enabled_mask & CAMERA_POWER_MASK_ALL) == CAMERA_POWER_MASK_ALL);
        return CameraPower_AppDriver_Set(CAMERA_POWER_ID_ALL, !all_enabled);
    }

    if (!CameraPower_IsValidSingleCamera(camera))
    {
        return CAMERA_POWER_STATUS_INVALID_CAMERA;
    }

    return CameraPower_AppDriver_Set(camera, !CameraPower_AppDriver_IsEnabled(camera));
}

bool CameraPower_AppDriver_IsEnabled(CameraPower_Id_t camera)
{
    if (!CameraPower_IsValidSingleCamera(camera))
    {
        return false;
    }

    return ((camera_power_enabled_mask & camera_power_pin_map[(uint8_t)camera].mask) != 0U);
}

uint8_t CameraPower_AppDriver_GetEnabledMask(void)
{
    return (camera_power_enabled_mask & CAMERA_POWER_MASK_ALL);
}
