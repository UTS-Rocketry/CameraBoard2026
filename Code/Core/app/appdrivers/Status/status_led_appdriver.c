#include "status_led_appdriver.h"

#include "main.h"

typedef struct
{
    uint16_t on_time_ms;
    uint16_t off_time_ms;
    bool steady_on;
} StatusLed_Pattern_t;

static const StatusLed_Pattern_t status_led_patterns[] =
{
    [STATUS_LED_STATE_OFF] = {0U, 0U, false},
    [STATUS_LED_STATE_BOOTING] = {100U, 100U, false},
    [STATUS_LED_STATE_IDLE] = {50U, 950U, false},
    [STATUS_LED_STATE_ARMED] = {250U, 250U, false},
    [STATUS_LED_STATE_RECORDING] = {0U, 0U, true},
    [STATUS_LED_STATE_ERROR] = {100U, 100U, false}
};

static StatusLed_State_t status_led_state = STATUS_LED_STATE_OFF;
static uint32_t status_led_last_transition_ms = 0U;
static bool status_led_is_on = false;

static bool StatusLed_IsValidState(StatusLed_State_t state)
{
    return ((uint32_t)state < (sizeof(status_led_patterns) / sizeof(status_led_patterns[0])));
}

static void StatusLed_Write(bool enabled)
{
    HAL_GPIO_WritePin(RGB_led_GPIO_Port,
                      RGB_led_Pin,
                      enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
    status_led_is_on = enabled;
}

void StatusLed_AppDriver_Init(void)
{
    status_led_state = STATUS_LED_STATE_OFF;
    status_led_last_transition_ms = HAL_GetTick();
    StatusLed_Write(false);
}

void StatusLed_AppDriver_SetState(StatusLed_State_t state)
{
    if (!StatusLed_IsValidState(state))
    {
        state = STATUS_LED_STATE_ERROR;
    }

    status_led_state = state;
    status_led_last_transition_ms = HAL_GetTick();

    if (state == STATUS_LED_STATE_OFF)
    {
        StatusLed_Write(false);
        return;
    }

    if (status_led_patterns[state].steady_on)
    {
        StatusLed_Write(true);
        return;
    }

    StatusLed_Write(true);
}

StatusLed_State_t StatusLed_AppDriver_GetState(void)
{
    return status_led_state;
}

void StatusLed_AppDriver_Set(bool enabled)
{
    status_led_state = enabled ? STATUS_LED_STATE_RECORDING : STATUS_LED_STATE_OFF;
    status_led_last_transition_ms = HAL_GetTick();
    StatusLed_Write(enabled);
}

void StatusLed_AppDriver_Toggle(void)
{
    StatusLed_Write(!status_led_is_on);
}

bool StatusLed_AppDriver_IsOn(void)
{
    return status_led_is_on;
}

void StatusLed_AppDriver_Task(uint32_t now_ms)
{
    if (!StatusLed_IsValidState(status_led_state))
    {
        StatusLed_AppDriver_SetState(STATUS_LED_STATE_ERROR);
        return;
    }

    const StatusLed_Pattern_t *pattern = &status_led_patterns[status_led_state];

    if (status_led_state == STATUS_LED_STATE_OFF)
    {
        StatusLed_Write(false);
        return;
    }

    if (pattern->steady_on)
    {
        StatusLed_Write(true);
        return;
    }

    const uint32_t elapsed_ms = now_ms - status_led_last_transition_ms;
    const uint32_t target_ms = status_led_is_on ? pattern->on_time_ms : pattern->off_time_ms;

    if (elapsed_ms >= target_ms)
    {
        status_led_last_transition_ms = now_ms;
        StatusLed_Write(!status_led_is_on);
    }
}
