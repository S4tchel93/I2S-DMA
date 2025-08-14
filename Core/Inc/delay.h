#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include "dsp_configuration.h"

#define DELAY_MAX_LENGTH 24000 // 1 second at 48kHz

typedef struct FX_Delay_t{
    float mix;
    float feedback;
    float line[DELAY_MAX_LENGTH];
    uint32_t lineIndex;

    uint32_t delayLength; // delay time == delay line length / sample rate

    float out;
}FX_Delay_t;

void    FX_Delay_Init(FX_Delay_t* dly, uint32_t delayTime_ms, float mix, float feedback);
float   FX_Do_Delay(FX_Delay_t* dly, float inSample);
void    FX_Delay_SetLength(FX_Delay_t* dly, uint32_t delayTime_ms);

#endif // DELAY_H