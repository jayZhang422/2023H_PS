/******************************************************************************
 * button_input.c
 *
 * Each API consumes one active-low press after debounce and release.
 ******************************************************************************/

#include "../include/app_config.h"
#include "../include/button_input.h"

#include "sleep.h"

static int button_input_take_press(button_input_t *buttons, u32 pin)
{
    if (XGpioPs_ReadPin(&buttons->instance, pin) !=
        APP_BUTTON_ACTIVE_LEVEL) {
        return 0;
    }

    usleep(APP_BUTTON_DEBOUNCE_US);
    if (XGpioPs_ReadPin(&buttons->instance, pin) !=
        APP_BUTTON_ACTIVE_LEVEL) {
        return 0;
    }

    while (XGpioPs_ReadPin(&buttons->instance, pin) ==
           APP_BUTTON_ACTIVE_LEVEL) {
        usleep(APP_BUTTON_POLL_US);
    }
    return 1;
}

int button_input_init(button_input_t *buttons)
{
    XGpioPs_Config *config;

    if (buttons == 0) {
        return XST_FAILURE;
    }

    config = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
    if (config == 0 || XGpioPs_CfgInitialize(&buttons->instance, config,
                                              config->BaseAddr) != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XGpioPs_SetDirectionPin(&buttons->instance, BUTTON_START, 0U);
    XGpioPs_SetDirectionPin(&buttons->instance, BUTTON_PHASE_ADJ, 0U);
    XGpioPs_SetOutputEnablePin(&buttons->instance, BUTTON_START, 0U);
    XGpioPs_SetOutputEnablePin(&buttons->instance, BUTTON_PHASE_ADJ, 0U);
    return XST_SUCCESS;
}

int button_input_take_start_press(button_input_t *buttons)
{
    return button_input_take_press(buttons, BUTTON_START);
}

int button_input_take_phase_press(button_input_t *buttons)
{
    return button_input_take_press(buttons, BUTTON_PHASE_ADJ);
}

u32 button_input_read_start_level(const button_input_t *buttons)
{
    return XGpioPs_ReadPin(&buttons->instance, BUTTON_START);
}

u32 button_input_read_phase_level(const button_input_t *buttons)
{
    return XGpioPs_ReadPin(&buttons->instance, BUTTON_PHASE_ADJ);
}
