#include "distortion.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------- Simple 1-pole LPF ----------

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

// ---------- Simple 1-pole HPF ----------


static void hpf_init(OnePoleHPF *f){ f->a0=1.0f; f->b1=0.0f; f->z1=0.0f; }

static void hpf_set(OnePoleHPF *f, float sample_rate, float cutoff_hz){
    if (cutoff_hz <= 0.0f || cutoff_hz >= sample_rate*0.45f){
        f->a0 = 1.0f; f->b1 = 0.0f; return; // bypass
    }
    float x = expf(-2.0f*M_PI*cutoff_hz/sample_rate);
    f->a0 = (1.0f + x) / 2.0f;
    f->b1 = x;
}

static inline float hpf_process(OnePoleHPF *f, float x){
    float y = f->a0 * (x - f->z1) + f->b1 * f->z1;
    f->z1 = x;
    return y;
}

// ---------- DS1 Effect ----------
void DS1_Init(DS1 *fx, float sample_rate){
    memset(fx, 0, sizeof(*fx));
    fx->sample_rate = sample_rate;
    fx->drive = 30.0f;
    fx->output = 1.0f;
    fx->type = CLIP_HARD;

    lpf_init(&fx->tone);
    lpf_set(&fx->tone, sample_rate, 6000.0f); // DS-1 tone LPF ~6 kHz

    hpf_init(&fx->hpf);
    hpf_set(&fx->hpf, sample_rate, 720.0f);   // DS-1 input HPF ~720 Hz
}

void DS1_SetParams(DS1 *fx, float drive, float output, float tone_hz, float hpf_hz, ClipType type){
    fx->drive = drive;
    fx->output = output;
    fx->type = type;
    lpf_set(&fx->tone, fx->sample_rate, tone_hz);
    hpf_set(&fx->hpf, fx->sample_rate, hpf_hz);
}

static inline float clip_sample(float x, ClipType type){
    switch(type){
        case CLIP_HARD:
            if (x > 0.3f) x = 0.3f;
            if (x < -0.3f) x = -0.3f;
            return x;
        case CLIP_TANH:
            return tanhf(x);
        case CLIP_ASYM:
            if (x > 0.3f) x = 0.3f;
            if (x < -0.2f) x = -0.2f;
            return x;
        default:
            return x;
    }
}

float DS1_ProcessSample(DS1 *fx, float in){
    // 1. Input HPF
    float x = hpf_process(&fx->hpf, in);

    // 2. Pre-gain
    x *= fx->drive;

    // 3. Clipping
    x = clip_sample(x, fx->type);

    // 4. Tone shaping LPF
    x = lpf_process(&fx->tone, x);

    // 5. Output level
    return x * fx->output;
}