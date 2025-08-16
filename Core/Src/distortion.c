#include "distortion.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ================= Utilities =================
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int g_fast_tanh = 1;
void Overdrive_SetFastTanh(int enable) { g_fast_tanh = enable ? 1 : 0; }

// ================= Biquad =================
typedef struct Biquad {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} Biquad;

static Biquad* biquad_new(void) {
    return (Biquad*)calloc(1, sizeof(Biquad));
}

static inline float biquad_process(Biquad *s, float x) {
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

static void biquad_reset(Biquad *s) {
    s->z1 = s->z2 = 0.0f;
}

static void biquad_make_lpf(Biquad *s, float fs, float fc, float Q) {
    float w0 = 2.0f * (float)M_PI * (fc / fs);
    float c = cosf(w0), sn = sinf(w0);
    float alpha = sn / (2.0f * Q);

    float b0 = (1.0f - c) * 0.5f;
    float b1 = 1.0f - c;
    float b2 = (1.0f - c) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;

    s->b0 = b0 / a0;
    s->b1 = b1 / a0;
    s->b2 = b2 / a0;
    s->a1 = a1 / a0;
    s->a2 = a2 / a0;
}

static void biquad_make_hpf(Biquad *s, float fs, float fc, float Q) {
    float w0 = 2.0f * (float)M_PI * (fc / fs);
    float c = cosf(w0), sn = sinf(w0);
    float alpha = sn / (2.0f * Q);

    float b0 = (1.0f + c) * 0.5f;
    float b1 = -(1.0f + c);
    float b2 = (1.0f + c) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;

    s->b0 = b0 / a0;
    s->b1 = b1 / a0;
    s->b2 = b2 / a0;
    s->a1 = a1 / a0;
    s->a2 = a2 / a0;
}

static void biquad_make_unity(Biquad *bq) {
    bq->b0 = 1.0f; bq->b1 = 0.0f; bq->b2 = 0.0f;
    bq->a1 = 0.0f; bq->a2 = 0.0f;
    biquad_reset(bq);
}

// ================= Waveshapers =================
static inline float fast_tanhf(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float clip_tanh(float x) {
    return g_fast_tanh ? fast_tanhf(x) : tanhf(x);
}

static inline float clip_atan(float x) {
    return (2.0f / (float)M_PI) * atanf(x);
}

static inline float waveshape(float x, float drive, ODClipType type) {
    float v = x * drive;
    switch (type) {
        case OD_CLIP_ATAN: return clip_atan(v);
        case OD_CLIP_TANH:
        default:           return clip_tanh(v);
    }
}

// ================= Overdrive Core =================
void Overdrive_DefaultParams(OverdriveParams *p) {
    p->drive       = 30.0f;
    p->output      = 1.0f;
    p->hpf_pre_Hz  = 60.0f;    // bass tighten before clip
    p->lpf_pre_Hz  = 0.0f;     // optional, default off
    p->lpf_post_Hz = 0.0f;     // optional “tone”, default off
    p->clip_type   = OD_CLIP_TANH;
}

void Overdrive_Init(Overdrive_t *od, float sample_rate_hz) {
    memset(od, 0, sizeof(*od));
    od->fs = sample_rate_hz;

    od->preHPF  = biquad_new();
    od->preLPF  = biquad_new();
    od->postLPF = biquad_new();

    OverdriveParams def;
    Overdrive_DefaultParams(&def);
    Overdrive_UpdateParams(od, &def);
}

void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p) {
    od->p = *p;
    float fs = od->fs;

    // Pre HPF always active
    biquad_make_hpf(od->preHPF, fs, fmaxf(5.0f, p->hpf_pre_Hz), 0.707f);

    // Pre LPF optional
    if (p->lpf_pre_Hz > 0.0f) {
        biquad_make_lpf(od->preLPF, fs, fminf(fs*0.45f, p->lpf_pre_Hz), 0.707f);
    } else {
        biquad_make_unity(od->preLPF);
    }

    // Post LPF optional
    if (p->lpf_post_Hz > 0.0f) {
        biquad_make_lpf(od->postLPF, fs, fminf(fs*0.45f, p->lpf_post_Hz), 0.707f);
    } else {
        biquad_make_unity(od->postLPF);
    }

    biquad_reset(od->preHPF);
    biquad_reset(od->preLPF);
    biquad_reset(od->postLPF);
}

float Overdrive_ProcessSample(Overdrive_t *od, float inSample) {
    // Pre EQ
    float v = biquad_process(od->preHPF, inSample);
    v = biquad_process(od->preLPF, v);

    // Waveshaper
    v = waveshape(v, od->p.drive, od->p.clip_type);

    // Post EQ
    v = biquad_process(od->postLPF, v);

    // Output level
    return v * od->p.output;
}