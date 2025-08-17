#include "dsp_configuration.h"
#include "audio_processing.h"
#include "reverb.h"
#include "delay.h"
#include "distortion.h"
#include "spring_verb.h"
#include "SEGGER_SYSVIEW.h"
#include <stdint.h>
#include <math.h>

/*rxBuf and txBuf are used for I2S DMA transfer, l_buf_in and
r_buf_in are used for processing input samples, l_buf_out and r_buf_out are used for processed output samples
rxBuf and txBuf are 16-bit buffers, l_buf_in, r_buf_in, l_buf_out and r_buf_out are 32-bit float buffers*/

/* Adjust if you prefer 20-bit scaling (some PCM1808 usages). 
   Default: use full 24-bit scale (2^23). */
#define INT24_SCALE_IN  8388608.0f    /* 2^23 */
#define INT24_SCALE_OUT 8388607.0f    /* 2^23 - 1 */

/*Buffers for DMA Transfer*/
static uint16_t rxBuf[BLOCK_SIZE_U16*2];
static uint16_t txBuf[BLOCK_SIZE_U16*2];
/*Audio Processing buffers*/
static float l_buf_in [BLOCK_SIZE_FLOAT*2];
static float r_buf_in [BLOCK_SIZE_FLOAT*2];
static float l_buf_out [BLOCK_SIZE_FLOAT*2];
static float r_buf_out [BLOCK_SIZE_FLOAT*2];

static I2S_DMA_Callback_State_t callback_state = I2S_DMA_CALLBACK_IDLE;
static FX_Delay_t dly_fx;
static DS1 ds1_fx;
static SpringReverb spring_reverb_fx;

#define SPRING_BUFFER_SIZE 8000
static float springBuffer[SPRING_BUFFER_SIZE];

void processAudio(void)
{
    int offset_r_ptr = 0;
    int offset_w_ptr = 0;
    int w_ptr = 0;

    if (callback_state != I2S_DMA_CALLBACK_IDLE) {
        SEGGER_SYSVIEW_RecordVoid(33);
        SEGGER_SYSVIEW_PrintfHost("DSP: Processing started, callback state: %d", callback_state);

        if (callback_state == I2S_DMA_CALLBACK_HALF) {
            offset_r_ptr = 0;
            offset_w_ptr = 0;
            w_ptr = 0;
        } else if (callback_state == I2S_DMA_CALLBACK_FULL) {
            offset_r_ptr = BLOCK_SIZE_U16;
            offset_w_ptr = BLOCK_SIZE_FLOAT;
            w_ptr = BLOCK_SIZE_FLOAT;
        }

        /* ---------- INPUT: rebuild signed 24-bit and normalize to [-1,1] ---------- */
        for (int i = offset_r_ptr; i < offset_r_ptr + BLOCK_SIZE_U16; i += 4) {
            /* Rebuild 32-bit left-justified frame from two 16-bit words (MSB first) */
            uint32_t rawL = ((uint32_t)rxBuf[i] << 16) | (uint32_t)rxBuf[i+1];
            uint32_t rawR = ((uint32_t)rxBuf[i+2] << 16) | (uint32_t)rxBuf[i+3];

            /* Cast to signed 32-bit then arithmetic shift right by 8 to get signed 24-bit value */
            int32_t s24L = ((int32_t)rawL) >> 8; // arithmetic shift preserves sign
            int32_t s24R = ((int32_t)rawR) >> 8;

            /* Normalize to float range [-1, +1] */
            l_buf_in[w_ptr] = (float)s24L / INT24_SCALE_IN;
            r_buf_in[w_ptr] = (float)s24R / INT24_SCALE_IN;

            w_ptr++;
        }

        /* ---------- PROCESS: per-sample DSP (expects normalized floats) ---------- */
        for (int i = offset_w_ptr; i < offset_w_ptr + BLOCK_SIZE_FLOAT; i++) {
            float inL = l_buf_in[i];
            float inR = r_buf_in[i];
            float temp_l = 0.0f;
            float temp_r = 0.0f;

            /* Call your DSP function (DS1 / Overdrive / etc) that expects normalized floats */
            //temp_l = DS1_ProcessSample(&ds1_fx, inL);
            //temp_r = DS1_ProcessSample(&ds1_fx, inR);

            /* Optionally chain other FX here */
            //temp_l = FX_Do_Delay(&dly_fx, temp_l);
            //temp_r = FX_Do_Delay(&dly_fx, temp_r);
            temp_l = SpringReverb_ProcessSample(&spring_reverb_fx, inL);
            temp_r = SpringReverb_ProcessSample(&spring_reverb_fx, inR);

            /* Store normalized result back (keep floats in [-1,1]) for clarity */
            l_buf_out[i] = temp_l;
            r_buf_out[i] = temp_r;
        }

        /* ---------- OUTPUT: convert normalized floats back to 24-bit MSB-aligned words ---------- */
        w_ptr = offset_w_ptr;
        for (int i = offset_r_ptr; i < offset_r_ptr + BLOCK_SIZE_U16; i += 4) {
            /* Round to nearest integer in 24-bit range (use INT24_SCALE_OUT) */
            float fl = l_buf_out[w_ptr];
            float fr = r_buf_out[w_ptr];

            /* clamp normalized floats just in case */
            if (fl > 1.0f) fl = 1.0f;
            if (fl < -1.0f) fl = -1.0f;
            if (fr > 1.0f) fr = 1.0f;
            if (fr < -1.0f) fr = -1.0f;

            int32_t s24L = (int32_t)lrintf(fl * INT24_SCALE_OUT); /* range -2^23..2^23-1 */
            int32_t s24R = (int32_t)lrintf(fr * INT24_SCALE_OUT);

            /* clamp to signed 24-bit just in case */
            if (s24L >  0x7FFFFF) s24L =  0x7FFFFF;
            if (s24L < -0x800000) s24L = -0x800000;
            if (s24R >  0x7FFFFF) s24R =  0x7FFFFF;
            if (s24R < -0x800000) s24R = -0x800000;

            /* Left-justify into 32-bit word (24-bit MSB-aligned) */
            uint32_t out32L = ((uint32_t)(s24L & 0xFFFFFF)) << 8;
            uint32_t out32R = ((uint32_t)(s24R & 0xFFFFFF)) << 8;

            /* Split into two 16-bit words for DMA */
            txBuf[i]   = (uint16_t)((out32L >> 16) & 0xFFFF);
            txBuf[i+1] = (uint16_t)(out32L & 0xFFFF);
            txBuf[i+2] = (uint16_t)((out32R >> 16) & 0xFFFF);
            txBuf[i+3] = (uint16_t)(out32R & 0xFFFF);

            w_ptr++;
        }

        callback_state = I2S_DMA_CALLBACK_IDLE;
        SEGGER_SYSVIEW_Print("DSP: Processing finished");
        SEGGER_SYSVIEW_RecordEndCall(33);
    }
}

void audio_SetCallbackState(I2S_DMA_Callback_State_t state)
{
    callback_state = state;
}

void audio_InitFX(void)
{
    Reverb_Init();
    SpringReverb_Init(&spring_reverb_fx, springBuffer, SPRING_BUFFER_SIZE, 0.5f, 0.3f);
    FX_Delay_Init(&dly_fx, 400, 0.25f, 0.5f); //500ms delay, 50% mix, 50% feedback
	DS1_Init(&ds1_fx, (float)SAMPLE_RATE); //Initialize overdrive with 48kHz sample rate
	DS1_SetParams(&ds1_fx, 60.0f, 2.0f, 4500.0f, 100.0f, CLIP_HARD); //Set parameters: drive=30, output=1, tone=6kHz, hpf=720Hz, clipping type=hard
}

uint16_t* audio_getTxBuf(void)
{
    return txBuf;
}
uint16_t* audio_getRxBuf(void)
{
    return rxBuf;
}

