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

#ifndef EI_MICROPHONE_H
#define EI_MICROPHONE_H

/* Include ----------------------------------------------------------------- */
#include <cstdint>
#include <cstdlib>

typedef int16_t microphone_sample_t;

/* Function prototypes ----------------------------------------------------- */
bool ei_microphone_sample_start(void);
bool ei_microphone_pdm_init(void);

bool ei_microphone_inference_start(uint32_t n_samples, float interval_ms);
bool ei_microphone_inference_record(void);
bool ei_microphone_inference_is_recording(void);
void ei_microphone_inference_reset_buffers(void);
int ei_microphone_inference_get_data(size_t offset, size_t length, float *out_ptr);
bool ei_microphone_inference_end(void);

#endif
