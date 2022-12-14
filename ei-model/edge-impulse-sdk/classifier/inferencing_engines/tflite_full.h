/*
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_FULL_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_FULL_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL)

#include "model-parameters/model_metadata.h"
#if EI_CLASSIFIER_HAS_MODEL_VARIABLES == 1
#include "model-parameters/model_variables.h"
#endif

#include <thread>
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/classifier/ei_model_types.h"

extern "C" EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    bool debug = false)
{

    static std::unique_ptr<tflite::FlatBufferModel> model = nullptr;
    static std::unique_ptr<tflite::Interpreter> interpreter = nullptr;

    if (!model) {
        model = tflite::FlatBufferModel::BuildFromBuffer((const char*)impulse->model_arr, impulse->model_arr_size);
        if (!model) {
            ei_printf("Failed to build TFLite model from buffer\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        tflite::ops::builtin::BuiltinOpResolver resolver;
        tflite::InterpreterBuilder builder(*model, resolver);
        builder(&interpreter);

        if (!interpreter) {
            ei_printf("Failed to construct interpreter\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        if (interpreter->AllocateTensors() != kTfLiteOk) {
            ei_printf("AllocateTensors failed\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        int hw_thread_count = (int)std::thread::hardware_concurrency();
        hw_thread_count -= 1; // leave one thread free for the other application
        if (hw_thread_count < 1) {
            hw_thread_count = 1;
        }

        if (interpreter->SetNumThreads(hw_thread_count) != kTfLiteOk) {
            ei_printf("SetNumThreads failed\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }
    }

    // Obtain pointers to the model's input and output tensors.
#if EI_CLASSIFIER_TFLITE_INPUT_QUANTIZED == 1
    int8_t* input = interpreter->typed_input_tensor<int8_t>(0);
#else
    float* input = interpreter->typed_input_tensor<float>(0);
#endif

    if (!input) {
        return EI_IMPULSE_INPUT_TENSOR_WAS_NULL;
    }

    for (uint32_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
    if (impulse->object_detection) {
#if EI_CLASSIFIER_TFLITE_INPUT_QUANTIZED == 1
        float pixel = (float)fmatrix->buffer[ix];
        input[ix] = static_cast<uint8_t>((pixel / impulse->tflite_input_scale) + impulse->tflite_input_zeropoint);
#else
        input[ix] = fmatrix->buffer[ix];
#endif
    }
    else {
#if EI_CLASSIFIER_TFLITE_INPUT_QUANTIZED == 1
        input[ix] = static_cast<int8_t>(round(fmatrix->buffer[ix] / impulse->tflite_input_scale) + impulse->tflite_input_zeropoint);
#else
        input[ix] = fmatrix->buffer[ix];
#endif
    }
    }

    uint64_t ctx_start_us = ei_read_timer_us();

    interpreter->Invoke();

    uint64_t ctx_end_us = ei_read_timer_us();

    result->timing.classification_us = ctx_end_us - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

#if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
    int8_t* out_data = interpreter->typed_output_tensor<int8_t>(impulse->tflite_output_data_tensor);
#else
    float* out_data = interpreter->typed_output_tensor<float>(impulse->tflite_output_data_tensor);
#endif

    if (!out_data) {
        return EI_IMPULSE_OUTPUT_TENSOR_WAS_NULL;
    }

    if (debug) {
        ei_printf("Predictions (time: %d ms.):\n", result->timing.classification);
    }

    if (impulse->object_detection == EI_OBJECT_DETECTION_FOMO) {
#if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
        fill_result_struct_i8_fomo(impulse, result, out_data, impulse->tflite_output_zeropoint, impulse->tflite_output_scale,
            impulse->input_width / 8, impulse->input_height / 8);
#else
        fill_result_struct_f32_fomo(impulse, result, out_data,
            impulse->input_width / 8, impulse->input_height / 8);
#endif
    }
    else if (impulse->object_detection == EI_OBJECT_DETECTION_SSD) {
        float *scores_tensor = interpreter->typed_output_tensor<float>(impulse->tflite_output_score_tensor);
        float *label_tensor = interpreter->typed_output_tensor<float>(impulse->tflite_output_labels_tensor);
        if (!scores_tensor) {
            return EI_IMPULSE_SCORE_TENSOR_WAS_NULL;
        }
        if (!label_tensor) {
            return EI_IMPULSE_LABEL_TENSOR_WAS_NULL;
        }
#if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
        ei_printf("ERR: MobileNet SSD does not support quantized inference.");
#else
        fill_result_struct_f32_object_detection(impulse, result, out_data, scores_tensor, label_tensor, debug);
#endif
    }
    else {
#if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
        fill_result_struct_i8(impulse, result, out_data, impulse->tflite_output_zeropoint, impulse->tflite_output_scale, debug);
#else
        fill_result_struct_f32(impulse, result, out_data, debug);
#endif
    }

    // on Linux we're not worried about free'ing (for now)

    return EI_IMPULSE_OK;
}

#endif // (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_FULL_H_
