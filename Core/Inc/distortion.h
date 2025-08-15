#ifndef DISTORTION_H
#define DISTORTION_H

#include <stdint.h>
//extern float Do_Distortion (float insample);

/* Input low pass filter fc = (fs/4), 1dB ripple, 60dB attenuation at stop-band, 53 taps*/
#define FX_OD_LPF_INP_LENGTH 53
extern float FX_OD_LPF_INP[FX_OD_LPF_INP_LENGTH];

typedef struct FX_Overdrive_t {
    float sampling_time;

    // Input Low pass filter
    float lpfInpBuf[FX_OD_LPF_INP_LENGTH];
    uint8_t lpfInpBufIndex;
    float lpfInpOut;

    // Input High pass filter
    float hpfInpBufIn[2];
    float hpfInpBufOut[2];
    float hpfInpWcT;
    float hpfInpOut;

    // Overdrive parameters
    float preGain;   // Input gain
    float threshold; // Clipping threshold

    /*output low pass filter*/
    float lpfOutBufIn[3];
    float lpfOutBufOut[3];
    float lpfOutWcT;
    float lpfOutDamp;
    float lpfOutOut;

    float out;

} FX_Overdrive_t;

void    FX_Overdrive_Init(FX_Overdrive_t* od, float hpfCutoffFrequencyHz, float odPreGain, float lpfCutoffFrequencyHz, float lpfDamping);
void    FX_Overdrive_SetHPF(FX_Overdrive_t* od, float hpfcutoffFrequencyHz);
void    FX_Overdrive_SetLPF(FX_Overdrive_t* od, float lpfCutoffFrequencyHz, float lpfDamping);
float   FX_Do_Overdrive(FX_Overdrive_t* od, float inSample);

#endif // DISTORTION_H