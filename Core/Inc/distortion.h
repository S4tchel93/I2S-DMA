#ifndef DISTORTION_H
#define DISTORTION_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    OD_CLIP_HARD = 0,   // DS1-style hard clipping
    OD_CLIP_TANH        // Soft clipping like TS8/Tube Screamer
} ODClipType;

typedef struct OverdriveParams {
    float drive;       // Pre-gain
    float output;      // Output trim
    float hpf_pre_Hz;  // Pre EQ HPF
    float lpf_pre_Hz;  // Pre EQ LPF (optional)
    float lpf_post_Hz; // Post EQ LPF (tone)
    ODClipType clip_type;
} OverdriveParams;

typedef struct Biquad {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} Biquad;

typedef struct Overdrive_t {
    float fs48;
    Biquad *preHPF;
    Biquad *preLPF;
    Biquad *postLPF;   // NEW
    OverdriveParams p;
} Overdrive_t;

// API
void Overdrive_SetFastTanh(int enable);
void Overdrive_DefaultParams(OverdriveParams *p);
void Overdrive_Init(Overdrive_t *od, float sample_rate_hz);
void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p);
float Overdrive_ProcessSample(Overdrive_t *od, float inSample);

#endif
