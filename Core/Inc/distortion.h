#ifndef OVERDRIVE_H
#define OVERDRIVE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OD_CLIP_TANH = 0,
    OD_CLIP_ATAN = 1
} ODClipType;

typedef struct {
    // User parameters
    float drive;        // Input gain into waveshaper (typ 1.0..20.0)
    float output;       // Output level trim (0..2)
    float hpf_pre_Hz;   // Pre-HPF cutoff (typ ~40 Hz)
    float lpf_pre_Hz;   // Pre-LPF cutoff (shape brightness into clip) (typ 4–8 kHz)
    float hpf_post_Hz;  // Post-HPF cutoff (typ 60–100 Hz)
    float lpf_post_Hz;  // Post-LPF cutoff (typ 5–8 kHz)
    float anti_fc_Hz;   // Anti-imaging/alias cutoff at 96 kHz (typ 18–20 kHz)
    ODClipType clip_type;
} OverdriveParams;

typedef struct {
    // Internal biquad state
    struct Biquad *preHPF;
    struct Biquad *preLPF;
    struct Biquad *antiLPF1_up;
    struct Biquad *antiLPF2_up;
    struct Biquad *antiLPF1_down;
    struct Biquad *antiLPF2_down;
    struct Biquad *postHPF;
    struct Biquad *postLPF;

    // Cached samplerates
    float fs48;
    float fs96;

    // Parameters
    OverdriveParams p;

} Overdrive_t;

void Overdrive_DefaultParams(OverdriveParams *p);
void Overdrive_Init(Overdrive_t *od, float sample_rate_hz);
void Overdrive_UpdateParams(Overdrive_t *od, const OverdriveParams *p);

float Overdrive_ProcessSample(Overdrive_t *od, float inSample);

// Optional: fast tanh approx enable (set nonzero to slightly faster, slightly less accurate)
void Overdrive_SetFastTanh(int enable);

#ifdef __cplusplus
}
#endif

#endif // OVERDRIVE_H