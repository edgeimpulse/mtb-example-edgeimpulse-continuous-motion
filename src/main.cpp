/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "firmware-sdk/ei_device_info_lib.h"
#include "cyhal_clock.h"
#include "cyhal_gpio.h"
#include "cyhal_uart.h"
#include "ei_device_psoc62.h"
#include "ei_at_handlers.h"
#include "ei_flash_memory.h"
#include "ei_inertial_sensor.h"
#include "ei_environment_sensor.h"
#include "ei_microphone.h"
#include "ei_run_impulse.h"

/******
 *
 * @brief EdgeImpulse PSoC6 firmware. See README.md for more information
 *
 ******/

/***************************************
*            Constants
****************************************/
#define UART_CLEAR_SCREEN   "\x1b[2J\x1b[;H"

/***************************************
*            Static Variables
****************************************/
static ATServer *at;
EiDevicePSoC62 *eidev;

void cy_err(int result)
{
    if(result != CY_RSLT_SUCCESS) {
        int err_type = CY_RSLT_GET_TYPE(result);
        int err_module = CY_RSLT_GET_MODULE(result);
        int err_code = CY_RSLT_GET_CODE(result);

        ei_printf("err type = 0x%X module = 0x%X code = 0x%X\n\r", err_type, err_module, err_code);
    }
}

void board_init(void)
{
    cy_rslt_t result;
    cyhal_clock_t system_clock, pll_clock, fll_clock;

    /* Make sure firmware starts after debugger */
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        Cy_SysLib_Delay(400u);
    }

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Initialize the PLL */
    cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[1]);
    cyhal_clock_set_frequency(&pll_clock, 150000000, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);
    cyhal_clock_free(&pll_clock);

    /* Initialize the system clock (HFCLK0) */
    cyhal_clock_reserve(&system_clock, &CYHAL_CLOCK_HF[0]);
    cyhal_clock_set_source(&system_clock, &pll_clock);
    cyhal_clock_free(&system_clock);

    /* Disable the FLL for power savings */
    cyhal_clock_reserve(&fll_clock, &CYHAL_CLOCK_FLL);
    cyhal_clock_set_enabled(&fll_clock, false, true);
    cyhal_clock_free(&fll_clock);

    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

int main(void)
{
	uint32_t led_state = CYBSP_LED_STATE_OFF;
	char uart_data;
    cy_rslt_t result;

    board_init();
    ei_inertial_sensor_init();
    ei_environment_sensor_init();
    ei_microphone_pdm_init();
    ei_printf(UART_CLEAR_SCREEN);

    eidev =  static_cast<EiDevicePSoC62*>(EiDeviceInfo::get_device());
    at = ei_at_init(eidev);
    ei_printf("Type AT+HELP to see a list of commands.\r\n");
    at->print_prompt();
    eidev->set_state(eiStateFinished);

    while(1)
    {
        if(cyhal_uart_getc(&cy_retarget_io_uart_obj, (uint8_t*)&uart_data, 5) == CY_RSLT_SUCCESS) {
            // controlling of the inference is a priority!
            if(is_inference_running() && uart_data == 'b') {
                ei_stop_impulse();
                at->print_prompt();
                continue;
            }

            // if no inference, then pass char to AT Server
            at->handle(uart_data);
        }

        if(is_inference_running()) {
            ei_run_impulse();
        }
    }
}
