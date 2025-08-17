#include "distortion.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------- Simple 1-pole lowpass filter ----------
typedef struct {
    float a0, b1, z1;
} OnePoleLPF;

static void lpf_init(OnePoleLPF *f){ f->a0=1.0f; f->b1=0.0f; f->z1=0.0f; }

static void lpf_set(OnePoleLPF *f, float sample_rate, float cutoff_hz){
    if (cutoff_hz <= 0.0f || cutoff_hz >= sample_rate*0.45f){
        f->a0 = 1.0f; f->b1 = 0.0f; return; // bypass
    }
    float x = expf(-2.0f*M_PI*cutoff_hz/sample_rate);
    f->a0 = 1.0f - x;
    f->b1 = x;
}

static inline float lpf_process(OnePoleLPF *f, float x){
    f->z1 = f->a0*x + f->b1*f->z1;
    return f->z1;
}

// ---------- DS1 Effect ----------
typedef struct {
    float drive;
    float output;
    OnePoleLPF tone;
} DS1_Impl;

void DS1_Init(DS1 *fx, float sample_rate){
    DS1_Impl *impl = (DS1_Impl*)fx;
    impl->drive = 30.0f;
    impl->output = 1.0f;
    lpf_init(&impl->tone);
    lpf_set(&impl->tone, sample_rate, 6000.0f); // DS-1 tone ~6kHz
}

void DS1_SetParams(DS1 *fx, float drive, float output, float tone_hz){
    DS1_Impl *impl = (DS1_Impl*)fx;
    impl->drive = drive;
    impl->output = output;
    lpf_set(&impl->tone, 48000.0f, tone_hz); // <- change fs if not 48k
}

float DS1_ProcessSample(DS1 *fx, float in){
    DS1_Impl *impl = (DS1_Impl*)fx;

    // 1. Pre-gain
    float x = in * impl->drive;

    // 2. Hard clip (DS1 diodes)
    if(x > 0.3f) x = 0.3f;
    if(x < -0.3f) x = -0.3f;

    // 3. Post "tone" LPF
    x = lpf_process(&impl->tone, x);

    // 4. Output trim
    return x * impl->output;
}
