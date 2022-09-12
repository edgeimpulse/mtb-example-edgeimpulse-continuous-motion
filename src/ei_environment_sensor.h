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

#ifndef EI_ENVIRONMENT_SENSOR_H
#define EI_ENVIRONMENT_SENSOR_H

/* Include ----------------------------------------------------------------- */
#include "firmware-sdk/ei_fusion.h"

/** Number of axis used and sample data format */
#define ENV_AXIS_SAMPLED    2

/* Function prototypes ----------------------------------------------------- */
bool ei_environment_sensor_init(void);
float *ei_fusion_environment_sensor_read_data(int n_samples);

static const ei_device_fusion_sensor_t environment_sensor = {
    // name of sensor module to be displayed in fusion list
    "Environmental",
    // number of sensor module axis
    ENV_AXIS_SAMPLED,
    // sampling frequencies
    { 10.0f, 20.0f, 62.5f },
    // axis name and units payload (must be same order as read in)
    { {"temperature", "degC"}, {"pressure", "kPa"} },
    // reference to read data function
    &ei_fusion_environment_sensor_read_data,
    0
};

#endif /* EI_ENVIRONMENT_SENSOR_H */
