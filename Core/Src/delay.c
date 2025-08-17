#include "delay.h"

void FX_Delay_Init(FX_Delay_t* dly, uint32_t delayTime_ms, float mix, float feedback) {
    FX_Delay_SetLength(dly, delayTime_ms);

    dly->mix = mix;
    dly->feedback = feedback;

    dly->lineIndex = 0;
    for (uint32_t i = 0; i < DELAY_MAX_LENGTH; i++) {
        dly->line[i] = 0.0f; // Initialize delay line
    }

    dly->out = 0.0f;
}

void FX_Delay_SetLength(FX_Delay_t* dly, uint32_t delayTime_ms) {
    if (delayTime_ms > 500) {
        delayTime_ms = 500; // Cap at 500 ms
    }
    dly->delayLength = (uint32_t)(SAMPLE_RATE * 0.001f * delayTime_ms);
    if (dly->delayLength > DELAY_MAX_LENGTH) {
        dly->delayLength = DELAY_MAX_LENGTH; // Ensure it does not exceed max length
    }
}

float FX_Do_Delay(FX_Delay_t* dly, float inSample) {
    // Read the delayed sample
    float delayLineOutput = dly->line[dly->lineIndex];

    // Compute current delay line input
    float delayLineInput = inSample + dly->feedback * delayLineOutput;

    // Write the new sample into the delay line circular buffer
    dly->line[dly->lineIndex] = delayLineInput;

    // Update the output sample
    dly->out = inSample * (1.0f - dly->mix) + (delayLineOutput * dly->mix);

    // Increment the index and wrap around if necessary
    dly->lineIndex++;
    if (dly->lineIndex >= dly->delayLength) {
        dly->lineIndex = 0;
    }

    if (dly->out < -1.0f) {
        dly->out = -1.0f; // Clamp output to -1.0
    } else if (dly->out > 1.0f) {
        dly->out = 1.0f; // Clamp output to 1.0
    }

    return dly->out;
}