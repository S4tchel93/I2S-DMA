#ifndef SPRING_REVERB_H
#define SPRING_REVERB_H

#include <stdint.h>

typedef struct {
    float *delayBuffer;
    uint32_t bufferSize;
    uint32_t writePos;

    float feedback;
    float mix;

    float allpass_z1;
    float allpass_coeff;
} SpringReverb;

// Initialize (allocate buffer externally, e.g. static float[...])
void SpringReverb_Init(SpringReverb *rv, float *buffer, uint32_t bufferSize, float feedback, float mix);

// Set parameters
void SpringReverb_SetParams(SpringReverb *rv, float feedback, float mix, float allpass_ms, float fs);

// Process single mono sample
float SpringReverb_ProcessSample(SpringReverb *rv, float in);

#endif
