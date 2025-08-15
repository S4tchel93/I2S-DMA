#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include <stdint.h>

typedef enum I2S_DMA_Callback_State_t {
  I2S_DMA_CALLBACK_IDLE = 0,
  I2S_DMA_CALLBACK_HALF = 1,
  I2S_DMA_CALLBACK_FULL = 2
} I2S_DMA_Callback_State_t;

extern void processAudio(void);

extern uint16_t* audio_getTxBuf(void);
extern uint16_t* audio_getRxBuf(void);
extern void audio_SetCallbackState(I2S_DMA_Callback_State_t state);
extern void audio_InitFX(void);

#endif // AUDIO_PROCESSING_H