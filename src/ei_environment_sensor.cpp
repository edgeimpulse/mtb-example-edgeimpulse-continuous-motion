/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdint.h>
#include <stdlib.h>

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "xensiv_dps3xx_mtb.h"

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "ei_environment_sensor.h"
#include "ei_device_psoc62.h"

/***************************************
*            Constants
****************************************/
#define I2C_FREQ_HZ (400000UL)
#define mI2C_SCL    CYBSP_I2C_SCL
#define mI2C_SDA    CYBSP_I2C_SDA
/* DPS310 library waits a certain time for one measurement to be ready
 * We use a wait time based on 128Hz sampling with 2x oversampling
 * settings */
#define DATA_WAITTIME (16U)

static float env_data[ENV_AXIS_SAMPLED];
static cyhal_i2c_t mI2C;
static cyhal_i2c_cfg_t i2c_config = {
    .is_slave = false,
    .address = 0,
    .frequencyhal_hz = I2C_FREQ_HZ
};
static xensiv_dps3xx_t pressure_sensor;
static xensiv_dps3xx_config_t dps310_config;


bool ei_environment_sensor_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    bool ret = false;

    ei_printf("\nInitializing DPS310 barometer\n");

    /* Initialize master i2c interface */
    result = cyhal_i2c_init(&mI2C, mI2C_SDA, mI2C_SCL, NULL);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    result = cyhal_i2c_configure(&mI2C, &i2c_config);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Initialize DPS310 sensor */
    result = xensiv_dps3xx_mtb_init_i2c(&pressure_sensor, &mI2C, XENSIV_DPS3XX_I2C_ADDR_DEFAULT);
    if (result != CY_RSLT_SUCCESS)
    {
        ei_printf("ERR: Barometer init failed (0x%04x)!\n", ret);
    }
    else {
        ret = true;
        ei_printf("Barometer is online\n");
    }

    result = xensiv_dps3xx_get_config(&pressure_sensor, &dps310_config);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    /* Set new sensor configuration - pressure first */
    dps310_config.dev_mode = XENSIV_DPS3XX_MODE_BACKGROUND_ALL;
    dps310_config.pressure_rate = XENSIV_DPS3XX_RATE_128;
    dps310_config.pressure_oversample = XENSIV_DPS3XX_OVERSAMPLE_2;
    dps310_config.data_timeout = DATA_WAITTIME;
    result = xensiv_dps3xx_set_config(&pressure_sensor, &dps310_config);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    /* Set new sensor configuration - temperature second */
    dps310_config.temperature_rate = XENSIV_DPS3XX_RATE_128;
    dps310_config.temperature_oversample = XENSIV_DPS3XX_OVERSAMPLE_2;
    result = xensiv_dps3xx_set_config(&pressure_sensor, &dps310_config);
    CY_ASSERT(result == CY_RSLT_SUCCESS);


    if(ei_add_sensor_to_fusion_list(environment_sensor) == false) {
        ei_printf("ERR: failed to register Environment sensor!\n");
        return false;
    }

    return ret;
}

bool ei_environment_sensor_test(void)
{
    cy_rslt_t result;
    bool ret = false;
    float pressure, temperature;

    result = xensiv_dps3xx_read(&pressure_sensor, &pressure, &temperature);
    if(result == CY_RSLT_SUCCESS) {
        ret = true;
        ei_printf("Pressure   : %8f\r\n", pressure);
        ei_printf("Temperature: %8f\r\n\r\n", temperature);
    }

    return ret;
}

float *ei_fusion_environment_sensor_read_data(int n_samples)
{
    cy_rslt_t result;
    float temp_data[ENV_AXIS_SAMPLED];
    float pressure, temperature;

    result = xensiv_dps3xx_read(&pressure_sensor, &pressure, &temperature);

    if(result == CY_RSLT_SUCCESS) {
        /* Conversion is done by the DPS3xx library */
        temp_data[0] = temperature;
        temp_data[1] = pressure;
        /* Any scaling can be done here */
        env_data[0] = temp_data[0];
        env_data[1] = temp_data[1];
    }
    else {
        env_data[0] = 0.0f;
        env_data[1] = 0.0f;
        ei_printf("ERR: no Environment data!\n");
    }

    return env_data;
}
