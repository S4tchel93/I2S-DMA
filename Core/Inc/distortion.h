#ifndef DS1_H
#define DS1_H

typedef struct {
    float drive;       // Pre-gain
    float output;      // Output volume trim
    float tone_hz;     // Post-LPF cutoff frequency
} DS1;

void DS1_Init(DS1 *fx, float sample_rate);
void DS1_SetParams(DS1 *fx, float drive, float output, float tone_hz);
float DS1_ProcessSample(DS1 *fx, float in);

#endif