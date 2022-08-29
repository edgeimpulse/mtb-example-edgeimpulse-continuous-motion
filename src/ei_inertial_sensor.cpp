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

#include <stdint.h>
#include <stdlib.h>

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "mtb_bmx160.h"

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "ei_inertial_sensor.h"
#include "ei_device_psoc62.h"

/***************************************
*            Constants
****************************************/
#define CONVERT_G_TO_MS2    9.80665f
#define IMU_SCALING_CONST   (16384.0)
/* SPI related */
#define SPI_FREQ_HZ         (8000000UL)
#define mSPI_MOSI           CYBSP_SPI_MOSI
#define mSPI_MISO           CYBSP_SPI_MISO
#define mSPI_SCLK           CYBSP_SPI_CLK
#define mSPI_SS             CYBSP_SPI_CS

static float imu_data[INERTIAL_AXIS_SAMPLED];
static cyhal_spi_t mSPI;
static mtb_bmx160_t motion_sensor;
static mtb_bmx160_data_t raw_data;

bool ei_inertial_sensor_init(void)
{
    cy_rslt_t result;
    bool ret = false;

    ei_printf("\nInitializing BMX160 accelerometer\n");

    /* Make sure BMX160 Accelerometer gets SS pin high early after power-up, so it switches to SPI mode */
    result = cyhal_gpio_init(mSPI_SS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* SPI chip select is controlled by the BMX160 driver, not the SPI controller */
    result = cyhal_spi_init(&mSPI, mSPI_MOSI, mSPI_MISO, mSPI_SCLK, NC, NULL, 8, CYHAL_SPI_MODE_00_MSB, false);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    result = cyhal_spi_set_frequency(&mSPI, SPI_FREQ_HZ);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    result = mtb_bmx160_init_spi(&motion_sensor, &mSPI, mSPI_SS);
    if (result != CY_RSLT_SUCCESS)
    {
        ei_printf("ERR: IMU init failed (0x%04x)!\n", ret);
    }
    else {
        ret = true;
        ei_printf("BMX160 IMU is online\n");
    }

    /* Default IMU range is 2g, therefore no need to do IMU configuration */

    if(ei_add_sensor_to_fusion_list(inertial_sensor) == false) {
        ei_printf("ERR: failed to register Inertial sensor!\n");
        return false;
    }


    return ret;
}

bool ei_inertial_sensor_test(void)
{
    cy_rslt_t result = mtb_bmx160_read(&motion_sensor, &raw_data);
    bool ret = false;

    if(result == CY_RSLT_SUCCESS) {
        ret = true;
        ei_printf("Accel: X:%6d Y:%6d Z:%6d\n", raw_data.accel.x, raw_data.accel.y, raw_data.accel.z);
        ei_printf("Gyro : X:%6d Y:%6d Z:%6d\n\n", raw_data.gyro.x, raw_data.gyro.y, raw_data.gyro.z);
        ei_printf("Mag  : X:%6d Y:%6d Z:%6d\n\n", raw_data.mag.x, raw_data.mag.y, raw_data.mag.z);
    }

    return ret;
}

float *ei_fusion_inertial_sensor_read_data(int n_samples)
{
    cy_rslt_t result;
    float temp_data[INERTIAL_AXIS_SAMPLED];

    result = mtb_bmx160_read(&motion_sensor, &raw_data);

    if(result == CY_RSLT_SUCCESS) {
        temp_data[0] = raw_data.accel.x / IMU_SCALING_CONST;
        temp_data[1] = raw_data.accel.y / IMU_SCALING_CONST;
        temp_data[2] = raw_data.accel.z / IMU_SCALING_CONST;

        imu_data[0] = temp_data[0] * CONVERT_G_TO_MS2;
        imu_data[1] = temp_data[1] * CONVERT_G_TO_MS2;
        imu_data[2] = temp_data[2] * CONVERT_G_TO_MS2;
    }
    else {
        ei_printf("ERR: no Accel data!\n");
        imu_data[0] = 0.0f;
        imu_data[1] = 0.0f;
        imu_data[2] = 0.0f;
    }

    return imu_data;
}
