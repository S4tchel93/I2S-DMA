#ifndef DISTORTION_H
#define DISTORTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ====== Clip Types ======
typedef enum {
    OD_CLIP_TANH = 0,
    OD_CLIP_ATAN = 1
} ODClipType;

// ====== User Parameters ======
typedef struct {
    float drive;        // input gain before waveshaper
    float output;       // output level
    float hpf_pre_Hz;   // pre high-pass cutoff (bass tighten)
    float lpf_pre_Hz;   // pre low-pass cutoff (optional, 0 = off)
    float lpf_post_Hz;  // post low-pass cutoff (tone, optional, 0 = off)
    ODClipType clip_type;
} OverdriveParams;

// ====== DSP State ======
typedef struct Biquad Biquad;  // opaque, defined in .c
typedef struct {
    float fs;             // sample rate
    OverdriveParams p;    // current parameters
    Biquad *preHPF;
    Biquad *preLPF;
    Biquad *postLPF;
} Overdrive_t;

// ====== API ======
void Overdrive_SetFastTanh(int enable);  // use fast tanh approx (default = 1)

void Overdrive_DefaultParams(OverdriveParams *p);
void Overdrive_Init(Overdrive_t *od, float sample_rate_hz);
void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p);

float Overdrive_ProcessSample(Overdrive_t *od, float inSample);

#ifdef __cplusplus
}
#endif

#endif // DISTORTION_H