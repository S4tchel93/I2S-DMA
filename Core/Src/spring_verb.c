#include "spring_verb.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void SpringReverb_Init(SpringReverb *rv, float *buffer, uint32_t bufferSize, float feedback, float mix) {
    rv->delayBuffer = buffer;
    rv->bufferSize = bufferSize;
    rv->writePos = 0;
    rv->feedback = feedback;
    rv->mix = mix;
    rv->allpass_z1 = 0.0f;
    rv->allpass_coeff = 0.5f;
    memset(buffer, 0, bufferSize * sizeof(float));
}

void SpringReverb_SetParams(SpringReverb *rv, float feedback, float mix, float allpass_ms, float fs) {
    rv->feedback = feedback;
    rv->mix = mix;
    float delaySamples = (allpass_ms / 1000.0f) * fs;
    // Convert to allpass coefficient
    rv->allpass_coeff = (delaySamples - 1.0f) / (delaySamples + 1.0f);
}

static inline float allpass_process(SpringReverb *rv, float x) {
    float y = -rv->allpass_coeff * x + rv->allpass_z1;
    rv->allpass_z1 = x + rv->allpass_coeff * y;
    return y;
}

float SpringReverb_ProcessSample(SpringReverb *rv, float in) {
    // Read position (circular buffer)
    uint32_t readPos = (rv->writePos + 1) % rv->bufferSize;
    float delayed = rv->delayBuffer[readPos];

    // Apply allpass (spring "boingy" feel)
    delayed = allpass_process(rv, delayed);

    // Feedback into delay buffer
    rv->delayBuffer[rv->writePos] = in + delayed * rv->feedback;

    // Advance circular buffer
    rv->writePos++;
    if (rv->writePos >= rv->bufferSize) rv->writePos = 0;

    // Mix dry + wet
    return (1.0f - rv->mix) * in + rv->mix * delayed;
}
