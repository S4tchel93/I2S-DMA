#include "distortion.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =================== Biquad ===================

static Biquad* biquad_new(void){
    Biquad *b = (Biquad*)calloc(1,sizeof(Biquad));
    return b;
}

static inline void biquad_reset(Biquad *s){
    s->z1 = s->z2 = 0.0f;
}

// RBJ high-pass
static void biquad_make_hpf(Biquad *s, float fs, float fc, float Q){
    float w0 = 2.0f * M_PI * fc / fs;
    float c = cosf(w0), sn = sinf(w0);
    float alpha = sn / (2.0f * Q);

    float b0 = (1.0f + c)/2.0f;
    float b1 = -(1.0f + c);
    float b2 = (1.0f + c)/2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;

    s->b0 = b0/a0; s->b1 = b1/a0; s->b2 = b2/a0;
    s->a1 = a1/a0; s->a2 = a2/a0;
    biquad_reset(s);
}

static void biquad_make_lpf(Biquad *s, float fs, float fc, float Q){
    float w0 = 2.0f * M_PI * fc / fs;
    float c = cosf(w0), sn = sinf(w0);
    float alpha = sn / (2.0f * Q);

    float b0 = (1.0f - c)/2.0f;
    float b1 = 1.0f - c;
    float b2 = (1.0f - c)/2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;

    s->b0 = b0/a0; s->b1 = b1/a0; s->b2 = b2/a0;
    s->a1 = a1/a0; s->a2 = a2/a0;
    biquad_reset(s);
}

static inline float biquad_process(Biquad *s, float x){
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

// =================== Overdrive ===================

void Overdrive_DefaultParams(OverdriveParams *p){
    p->drive = 6.0f;          // pre-gain
    p->output = 5.0f;         // output trim
    p->hpf_pre_Hz = 60.0f;    // pre HPF
    p->lpf_pre_Hz = 0.0f;     // optional LPF disabled
    p->clip_type = OD_CLIP_HARD;
}

void Overdrive_Init(Overdrive_t *od, float sample_rate_hz){
    memset(od,0,sizeof(*od));
    od->fs48 = sample_rate_hz;
    od->preHPF = biquad_new();
    od->preLPF = biquad_new();
    OverdriveParams def;
    Overdrive_DefaultParams(&def);
    Overdrive_UpdateParams(od,&def);
}

void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p){
    od->p = *p;
    float fs = od->fs48;

    // Pre HPF
    biquad_make_hpf(od->preHPF, fs, fmaxf(5.0f,p->hpf_pre_Hz),0.707f);

    // Pre LPF optional
    if(p->lpf_pre_Hz > 0.0f){
        biquad_make_lpf(od->preLPF, fs, fminf(fs*0.45f,p->lpf_pre_Hz),0.707f);
    }else{
        od->preLPF->b0=1.0f; od->preLPF->b1=od->preLPF->b2=0.0f;
        od->preLPF->a1=od->preLPF->a2=0.0f;
        biquad_reset(od->preLPF);
    }
}

// Per-sample processing with optional hard/soft clipping
float Overdrive_ProcessSample(Overdrive_t *od, float inSample){
    // 1. Pre EQ
    float x = biquad_process(od->preHPF, inSample);
    x = biquad_process(od->preLPF, x);

    // 2. Pre-gain
    float y = x * od->p.drive;

    // 3. Clipping
    switch(od->p.clip_type){
        case OD_CLIP_HARD: {
            float maxVal = 8388608.0f; // 24-bit MSB-aligned
            if(y > maxVal) y = maxVal;
            if(y < -maxVal) y = -maxVal;
            break;
        }
        case OD_CLIP_TANH:
        default: {
            // Soft saturation (like a tube screamer)
            y = y / 8388608.0f;          // normalize to [-1,1)
            y = tanhf(y);                 // soft tanh
            y *= 8388608.0f;             // back to MSB-aligned float
            break;
        }
    }

    // 4. Output trim
    y *= od->p.output;

    return y;
}
