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

// #ifndef _EDGE_IMPULSE_SIGNING_MBEDTLS_HMAC_SHA256_H_
// #define _EDGE_IMPULSE_SIGNING_MBEDTLS_HMAC_SHA256_H_

/**
 * HMAC SHA256 implementation using Mbed TLS
 */

#include <string.h>
#include "firmware-sdk/sensor_aq.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
//#include "mbed_trace.h"
#include "ei_mbedtls_md.h"

//#ifdef MBEDTLS_MD_C
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

typedef struct {
    mbedtls_md_context_t md_ctx;
    char hmac_key[33];
} sensor_aq_mbedtls_hs256_ctx_t;

static int sensor_aq_mbedtls_hs256_init(sensor_aq_signing_ctx_t *aq_ctx) {
    sensor_aq_mbedtls_hs256_ctx_t *hs_ctx = (sensor_aq_mbedtls_hs256_ctx_t*)aq_ctx->ctx;

    int err;

    //mbedtls_md_init(&hs_ctx->md_ctx);
    memset(&hs_ctx->md_ctx, 0, sizeof( mbedtls_md_context_t ) );

    err = ei_mbedtls_md_setup(&hs_ctx->md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256) , 1); //use hmac
    if (err != 0) {
        return err;
    }

    return ei_mbedtls_md_hmac_starts(&hs_ctx->md_ctx, (const unsigned char *)hs_ctx->hmac_key, strlen(hs_ctx->hmac_key));
}

static int sensor_aq_mbedtls_hs256_update(sensor_aq_signing_ctx_t *aq_ctx, const uint8_t *buffer, size_t buffer_size) {
    sensor_aq_mbedtls_hs256_ctx_t *hs_ctx = (sensor_aq_mbedtls_hs256_ctx_t*)aq_ctx->ctx;

    return ei_mbedtls_md_hmac_update(&hs_ctx->md_ctx, buffer, buffer_size);
}

static int sensor_aq_mbedtls_hs256_finish(sensor_aq_signing_ctx_t *aq_ctx, uint8_t *buffer) {
    sensor_aq_mbedtls_hs256_ctx_t *hs_ctx = (sensor_aq_mbedtls_hs256_ctx_t*)aq_ctx->ctx;

    int ret = ei_mbedtls_md_hmac_finish(&hs_ctx->md_ctx, buffer);
    mbedtls_md_free(&hs_ctx->md_ctx);
    return ret;
}

/**
 * Construct a new signing context for HMAC SHA256 using Mbed TLS
 *
 * @param aq_ctx An empty signing context (can declare it without arguments)
 * @param hs_ctx An empty sensor_aq_mbedtls_hs256_ctx_t context (can declare it on the stack without arguments)
 * @param hmac_key The secret key - **NOTE: this is limited to 32 characters, the rest will be truncated**
 */
void sensor_aq_init_mbedtls_hs256_context(sensor_aq_signing_ctx_t *aq_ctx, sensor_aq_mbedtls_hs256_ctx_t *hs_ctx, const char *hmac_key) {
    memcpy(hs_ctx->hmac_key, hmac_key, 32);
    hs_ctx->hmac_key[32] = 0;

    if (strlen(hmac_key) > 32) {
        ei_printf("!!! sensor_aq_init_mbedtls_hs256_context, HMAC key is longer than 32 characters - will be truncated !!!\n");
    }

    aq_ctx->alg = "HS256"; // JWS algorithm
    aq_ctx->signature_length = 32;
    aq_ctx->ctx = (void*)hs_ctx;
    aq_ctx->init = &sensor_aq_mbedtls_hs256_init;
    aq_ctx->set_protected = NULL;
    aq_ctx->update = &sensor_aq_mbedtls_hs256_update;
    aq_ctx->finish = &sensor_aq_mbedtls_hs256_finish;
}

// #else

// #error "sensor_aq_mbedtls_hs256 loaded but Mbed TLS was not found, or MBEDTLS_MD_C was disabled"

// #endif // MBEDTLS_MD_C

// #endif // _EDGE_IMPULSE_SIGNING_MBEDTLS_HMAC_SHA256_H_
