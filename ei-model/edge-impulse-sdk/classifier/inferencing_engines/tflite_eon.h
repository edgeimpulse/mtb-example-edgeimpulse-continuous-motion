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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_EON_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_EON_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE) && (EI_CLASSIFIER_COMPILED == 1)

#include "model-parameters/model_metadata.h"
#if EI_CLASSIFIER_HAS_MODEL_VARIABLES == 1
#include "model-parameters/model_variables.h"
#endif

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tflite-model/trained_model_compiled.h"
#include "edge-impulse-sdk/classifier/ei_aligned_malloc.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/classifier/ei_model_types.h"

#if defined(EI_CLASSIFIER_ENABLE_DETECTION_POSTPROCESS_OP)
namespace tflite {
namespace ops {
namespace micro {
extern TfLiteRegistration Register_TFLite_Detection_PostProcess(void);
}  // namespace micro
}  // namespace ops

extern float post_process_boxes[10 * 4 * sizeof(float)];
extern float post_process_classes[10];
extern float post_process_scores[10];

}  // namespace tflite

static TfLiteRegistration post_process_op = tflite::ops::micro::Register_TFLite_Detection_PostProcess();

#endif // defined(EI_CLASSIFIER_ENABLE_DETECTION_POSTPROCESS_OP)


/**
 * Setup the TFLite runtime
 *
 * @param      ctx_start_us       Pointer to the start time
 * @param      input              Pointer to input tensor
 * @param      output             Pointer to output tensor
 * @param      micro_tensor_arena Pointer to the arena that will be allocated
 *
 * @return  EI_IMPULSE_OK if successful
 */
static EI_IMPULSE_ERROR inference_tflite_setup(const ei_impulse_t *impulse, uint64_t *ctx_start_us, TfLiteTensor** input, TfLiteTensor** output,
    TfLiteTensor** output_labels,
    TfLiteTensor** output_scores,
    ei_unique_ptr_t& p_tensor_arena) {

    TfLiteStatus init_status = trained_model_init(ei_aligned_calloc);
    if (init_status != kTfLiteOk) {
        ei_printf("Failed to allocate TFLite arena (error code %d)\n", init_status);
        return EI_IMPULSE_TFLITE_ARENA_ALLOC_FAILED;
    }

    *ctx_start_us = ei_read_timer_us();

    static bool tflite_first_run = true;

    *input = impulse->model_input(0);
    *output = impulse->model_output(0);

    if (impulse->object_detection) {
        *output_scores = impulse->model_output(impulse->tflite_output_score_tensor);
        *output_labels = impulse->model_output(impulse->tflite_output_labels_tensor);
    }

    // Assert that our quantization parameters match the model
    if (tflite_first_run) {
        assert((*input)->type == impulse->tflite_input_datatype);
        assert((*output)->type == impulse->tflite_output_datatype);
        if (impulse->object_detection) {
            assert((*output_scores)->type == impulse->tflite_output_datatype);
            assert((*output_labels)->type == impulse->tflite_output_datatype);
        }
        if (impulse->tflite_input_quantized) {
            assert((*input)->params.scale == impulse->tflite_input_scale);
            assert((*input)->params.zero_point == impulse->tflite_input_zeropoint);
        }
        if (impulse->tflite_output_quantized) {
            assert((*output)->params.scale == impulse->tflite_output_scale);
            assert((*output)->params.zero_point == impulse->tflite_output_zeropoint);
        }
        tflite_first_run = false;
    }
    return EI_IMPULSE_OK;
}

/**
 * Run TFLite model
 *
 * @param   ctx_start_us    Start time of the setup function (see above)
 * @param   output          Output tensor
 * @param   interpreter     TFLite interpreter (non-compiled models)
 * @param   tensor_arena    Allocated arena (will be freed)
 * @param   result          Struct for results
 * @param   debug           Whether to print debug info
 *
 * @return  EI_IMPULSE_OK if successful
 */
static EI_IMPULSE_ERROR inference_tflite_run(const ei_impulse_t *impulse,
    uint64_t ctx_start_us,
    TfLiteTensor* output,
    TfLiteTensor* labels_tensor,
    TfLiteTensor* scores_tensor,
    uint8_t* tensor_arena,
    ei_impulse_result_t *result,
    bool debug) {

    if(trained_model_invoke() != kTfLiteOk) {
        return EI_IMPULSE_TFLITE_ERROR;
    }

    uint64_t ctx_end_us = ei_read_timer_us();

    result->timing.classification_us = ctx_end_us - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

    // Read the predicted y value from the model's output tensor
    if (debug) {
        ei_printf("Predictions (time: %d ms.):\n", result->timing.classification);
    }

    if (impulse->object_detection == EI_OBJECT_DETECTION_FOMO) {
        bool int8_output = output->type == TfLiteType::kTfLiteInt8;
        if (int8_output) {
            fill_result_struct_i8_fomo(impulse, result, output->data.int8, output->params.zero_point, output->params.scale,
                (int)output->dims->data[1], (int)output->dims->data[2]);
        }
        else {
            fill_result_struct_f32_fomo(impulse, result, output->data.f, (int)output->dims->data[1], (int)output->dims->data[2]);
        }
    }
    else if (impulse->object_detection == EI_OBJECT_DETECTION_SSD) {
#if EI_CLASSIFIER_ENABLE_DETECTION_POSTPROCESS_OP
        fill_result_struct_f32_object_detection(impulse, result, tflite::post_process_boxes, tflite::post_process_scores, tflite::post_process_classes, debug);
#else
        ei_printf("WARNING: Object detection post-processing op is not enabled.");
#endif
        // fill_result_struct_f32(result, output->data.f, scores_tensor->data.f, labels_tensor->data.f, debug);
    }
    else {
        bool int8_output = output->type == TfLiteType::kTfLiteInt8;
        if (int8_output) {
            fill_result_struct_i8(impulse, result, output->data.int8, output->params.zero_point, output->params.scale, debug);
        }
        else {
            fill_result_struct_f32(impulse, result, output->data.f, debug);
        }
    }

    trained_model_reset(ei_aligned_free);

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    return EI_IMPULSE_OK;
}


/**
 * @brief      Do neural network inferencing over the processed feature matrix
 *
 * @param      fmatrix  Processed matrix
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    bool debug = false)
{
    TfLiteTensor* input;
    TfLiteTensor* output;
    TfLiteTensor* output_scores;
    TfLiteTensor* output_labels;

    uint64_t ctx_start_us = ei_read_timer_us();
    ei_unique_ptr_t p_tensor_arena(nullptr, ei_aligned_free);

    EI_IMPULSE_ERROR init_res = inference_tflite_setup(impulse,
        &ctx_start_us, 
        &input, &output,
        &output_labels,
        &output_scores,
        p_tensor_arena);

    if (init_res != EI_IMPULSE_OK) {
        return init_res;
    }

    uint8_t* tensor_arena = static_cast<uint8_t*>(p_tensor_arena.get());

    if (impulse->object_detection) {
        // Place our calculated x value in the model's input tensor
        bool uint8_input = input->type == TfLiteType::kTfLiteUInt8;
        for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
            if (uint8_input) {
                float pixel = (float)fmatrix->buffer[ix];
                input->data.uint8[ix] = static_cast<uint8_t>((pixel / impulse->tflite_input_scale) + impulse->tflite_input_zeropoint);
            }
            else {
                input->data.f[ix] = fmatrix->buffer[ix];
            }
        }
    }
    else {
        bool int8_input = input->type == TfLiteType::kTfLiteInt8;
        for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
            // Quantize the input if it is int8
            if (int8_input) {
                input->data.int8[ix] = static_cast<int8_t>(round(fmatrix->buffer[ix] / input->params.scale) + input->params.zero_point);
                // printf("float %ld : %d\r\n", ix, input->data.int8[ix]);
            } else {
                input->data.f[ix] = fmatrix->buffer[ix];
            }
        }
    }

    EI_IMPULSE_ERROR run_res = inference_tflite_run(impulse, ctx_start_us,
                                                    output, output_labels, output_scores,
                                                    tensor_arena, result, debug);

    result->timing.classification_us = ei_read_timer_us() - ctx_start_us;

    if (run_res != EI_IMPULSE_OK) {
        return run_res;
    }

    return EI_IMPULSE_OK;
}

#if EI_CLASSIFIER_TFLITE_INPUT_QUANTIZED == 1
/**
 * Special function to run the classifier on images, only works on TFLite models (either interpreter or EON or for tensaiflow)
 * that allocates a lot less memory by quantizing in place. This only works if 'can_run_classifier_image_quantized'
 * returns EI_IMPULSE_OK.
 */
EI_IMPULSE_ERROR run_nn_inference_image_quantized(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false) {

    memset(result, 0, sizeof(ei_impulse_result_t));

    uint64_t ctx_start_us;
    TfLiteTensor* input;
    TfLiteTensor* output;
    TfLiteTensor* output_scores;
    TfLiteTensor* output_labels;

    ei_unique_ptr_t p_tensor_arena(nullptr, ei_aligned_free);

    EI_IMPULSE_ERROR init_res = inference_tflite_setup(impulse,
        &ctx_start_us, &input, &output,
        &output_labels,
        &output_scores,
        p_tensor_arena);
    if (init_res != EI_IMPULSE_OK) {
        return init_res;
    }

    if (input->type != TfLiteType::kTfLiteInt8) {
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

    uint64_t dsp_start_us = ei_read_timer_us();

    // features matrix maps around the input tensor to not allocate any memory
    ei::matrix_i8_t features_matrix(1, impulse->nn_input_frame_size, input->data.int8);

    // run DSP process and quantize automatically
    int ret = extract_image_features_quantized(impulse, signal, &features_matrix, ei_dsp_blocks[0].config, impulse->frequency);
    if (ret != EIDSP_OK) {
        ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
        return EI_IMPULSE_DSP_ERROR;
    }

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("Features (%d ms.): ", result->timing.dsp);
        for (size_t ix = 0; ix < features_matrix.cols; ix++) {
            ei_printf_float((features_matrix.buffer[ix] - impulse->tflite_input_zeropoint) * impulse->tflite_input_scale);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    ctx_start_us = ei_read_timer_us();

    EI_IMPULSE_ERROR run_res = inference_tflite_run(impulse,
        ctx_start_us, 
        output,
        output_labels,
        output_scores,
        static_cast<uint8_t*>(p_tensor_arena.get()),
        result, debug);

    if (run_res != EI_IMPULSE_OK) {
        return run_res;
    }

    result->timing.classification_us = ei_read_timer_us() - ctx_start_us;

    return EI_IMPULSE_OK;
}
#endif // EI_CLASSIFIER_TFLITE_INPUT_QUANTIZED == 1

#endif // (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE) && (EI_CLASSIFIER_COMPILED == 1)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_EON_H_
