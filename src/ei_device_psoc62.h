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

#ifndef EI_DEVICE_PSOC62_H_
#define EI_DEVICE_PSOC62_H_

#include "firmware-sdk/ei_device_info_lib.h"
#include "firmware-sdk/ei_device_memory.h"
#include "cyhal_timer.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* Defines related to non-fusion sensors
 *
 * Note: Number of fusion sensors can be found in ei_fusion_sensors_config.h
 *
 */
#define EI_STANDALONE_SENSOR_MIC 0
#define EI_STANDALONE_SENSORS_COUNT 1

#define EI_DEVICE_BAUDRATE 115200
#define EI_DEVICE_BAUDRATE_MAX 460800

class EiDevicePSoC62 : public EiDeviceInfo {
private:
    EiDevicePSoC62() = delete;
    static const int sensors_count = EI_STANDALONE_SENSORS_COUNT;
    ei_device_sensor_t sensors[sensors_count];
    EiState state;
    cyhal_timer_t sample_timer;
    cyhal_timer_cfg_t sample_timer_cfg;
    cyhal_timer_t led_timer;

public:
    EiDevicePSoC62(EiDeviceMemory *mem);
    ~EiDevicePSoC62();
    void init_device_id(void);
    void clear_config(void);
    bool get_sensor_list(const ei_device_sensor_t **sensor_list, size_t *sensor_list_size) override;
    uint32_t get_data_output_baudrate(void);
    void set_max_data_output_baudrate(void);
    void set_default_data_output_baudrate(void);
    bool test_flash(void);
    bool start_sample_thread(void (*sample_read_cb)(void), float sample_interval_ms) override;
    bool stop_sample_thread(void) override;
    void set_state(EiState state) override;
    EiState get_state(void);
};

#endif /* EI_DEVICE_PSOC62_H_ */
