#include "distortion.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ================= Utilities & Config =================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int g_fast_tanh = 1;
void Overdrive_SetFastTanh(int enable){ g_fast_tanh = enable ? 1 : 0; }

// ================= Biquad IIR =================

typedef struct Biquad {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} Biquad;

static Biquad* biquad_new(void){
    Biquad *b = (Biquad*)calloc(1, sizeof(Biquad));
    return b;
}

static inline float biquad_process(Biquad *s, float x)
{
    // Direct Form I Transposed
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

static void biquad_reset(Biquad *s){
    s->z1 = s->z2 = 0.0f;
}

// RBJ cookbook coefficient builders (single-precision)
static void biquad_make_lpf(Biquad *s, float fs, float fc, float Q)
{
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

static void biquad_make_hpf(Biquad *s, float fs, float fc, float Q)
{
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

// Unity biquad (passes through untouched)
static void biquad_make_unity(Biquad *bq) {
    bq->b0 = 1.0f; bq->b1 = 0.0f; bq->b2 = 0.0f;
    bq->a1 = 0.0f; bq->a2 = 0.0f;
    biquad_reset(bq);
}

// ================= Waveshapers =================

static inline float fast_tanhf(float x)
{
    // 3rd-order tanh approx: x*(27 + x^2)/(27 + 9x^2)
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float clip_tanh(float x)
{
    return g_fast_tanh ? fast_tanhf(x) : tanhf(x);
}

static inline float clip_atan(float x)
{
    // normalized arctan clip to ~[-1,1] at large |x|
    return (2.0f / (float)M_PI) * atanf(x);
}

static inline float waveshape(float x, float drive, ODClipType type)
{
    float v = x * drive;
    switch(type){
        case OD_CLIP_ATAN: return clip_atan(v);
        case OD_CLIP_TANH:
        default:           return clip_tanh(v);
    }
}

// ================= Overdrive Core =================

void Overdrive_DefaultParams(OverdriveParams *p)
{
    p->drive       = 30.0f;     // how hard we hit the clipper
    p->output      = 1.0f;     // makeup
    p->hpf_pre_Hz  = 60.0f;    // bass tighten before clip
    p->lpf_pre_Hz  = 0.0f;     // 0 = disabled (optional extra tighten)
    p->hpf_post_Hz = 10.0f;    // tiny DC cleanup
    p->lpf_post_Hz = 10000.0f;  // “Tone” control (fizz tamer)
    p->anti_fc_Hz  = 0.0f;     // unused in this simplified path
    p->clip_type   = OD_CLIP_TANH;
}

void Overdrive_Init(Overdrive_t *od, float sample_rate_hz)
{
    memset(od, 0, sizeof(*od));
    od->fs48 = sample_rate_hz;
    od->fs96 = sample_rate_hz * 2.0f;

    // allocate filters
    od->preHPF       = biquad_new();
    od->preLPF       = biquad_new();
    od->antiLPF1_up  = biquad_new();
    od->antiLPF2_up  = biquad_new();
    od->antiLPF1_down= biquad_new();
    od->antiLPF2_down= biquad_new();
    od->postHPF      = biquad_new();
    od->postLPF      = biquad_new();

    OverdriveParams def;
    Overdrive_DefaultParams(&def);
    Overdrive_UpdateParams(od, &def);
}

void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p)
{
     od->p = *p;

    // sample rates already set in Init
    float fs = od->fs48;

    // Pre high-pass (always on)
    biquad_make_hpf(od->preHPF, fs, fmaxf(5.0f, p->hpf_pre_Hz), 0.707f);

    // Optional pre low-pass (0 disables)
    if (p->lpf_pre_Hz > 0.0f) {
        biquad_make_lpf(od->preLPF, fs, fminf(fs*0.45f, p->lpf_pre_Hz), 0.707f);
    } else {
        // neutralize
        od->preLPF->b0 = 1.0f; od->preLPF->b1 = od->preLPF->b2 = 0.0f;
        od->preLPF->a1 = od->preLPF->a2 = 0.0f;
        biquad_reset(od->preLPF);
    }

    // Post tone LPF (single tone control)
    if (p->lpf_post_Hz > 0.0f) {
        biquad_make_lpf(od->postLPF, fs, fminf(fs*0.45f, p->lpf_post_Hz), 0.707f);
    } else {
        biquad_make_unity(od->postLPF);
    }

    // Tiny post HPF to remove DC
    biquad_make_hpf(od->postHPF, fs, fmaxf(5.0f, p->hpf_post_Hz), 0.707f);

    // Disable post HPF completely
    biquad_make_unity(od->postHPF);

    // Clear states to avoid “stuck” old coefficients
    biquad_reset(od->preHPF);
    biquad_reset(od->preLPF);
    biquad_reset(od->postLPF);
    biquad_reset(od->postHPF);
}

// Per-sample processing with 2× oversampling inside
float Overdrive_ProcessSample(Overdrive_t *od, float inSample)
{
    // Pre EQ
    float v = biquad_process(od->preHPF, inSample);
    v = biquad_process(od->preLPF, v); // no-op if disabled

    // Clipper
    v = waveshape(v, od->p.drive, od->p.clip_type);

    // Post EQ (tone + DC)
    //v = biquad_process(od->postLPF, v);
    //v = biquad_process(od->postHPF, v);

    // Output level
    v *= od->p.output;

    // No hard limiter here — let the waveshaper be the limiter.
    // If you want a gentle ceiling, uncomment:
    // v = clip_tanh(v);

    return v;
}