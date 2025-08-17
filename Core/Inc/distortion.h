#ifndef DS1_H
#define DS1_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float a0, b1, z1;
} OnePoleLPF;

typedef struct {
    float a0, b1, z1;
} OnePoleHPF;

typedef enum {
    CLIP_HARD,
    CLIP_TANH,
    CLIP_ASYM
} ClipType;

typedef struct DS1 {
    float drive;
    float output;
    ClipType type;
    OnePoleLPF tone;
    OnePoleHPF hpf;
    float sample_rate;
} DS1;


void DS1_Init(DS1 *fx, float sample_rate);
void DS1_SetParams(DS1 *fx, float drive, float output, float tone_hz, float hpf_hz, ClipType type);
float DS1_ProcessSample(DS1 *fx, float in);

#ifdef __cplusplus
}
#endif

#endif // DS1_H