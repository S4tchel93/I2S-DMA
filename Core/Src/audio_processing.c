#include "dsp_configuration.h"
#include "audio_processing.h"
#include "reverb.h"
#include "delay.h"
#include "SEGGER_SYSVIEW.h"

/*rxBuf and txBuf are used for I2S DMA transfer, l_buf_in and
r_buf_in are used for processing input samples, l_buf_out and r_buf_out are used for processed output samples
rxBuf and txBuf are 16-bit buffers, l_buf_in, r_buf_in, l_buf_out and r_buf_out are 32-bit float buffers*/

/*Buffers for DMA Transfer*/
static uint16_t rxBuf[BLOCK_SIZE_U16*2];
static uint16_t txBuf[BLOCK_SIZE_U16*2];
/*Audio Processing buffers*/
static float l_buf_in [BLOCK_SIZE_FLOAT*2];
static float r_buf_in [BLOCK_SIZE_FLOAT*2];
static float l_buf_out [BLOCK_SIZE_FLOAT*2];
static float r_buf_out [BLOCK_SIZE_FLOAT*2];

static I2S_DMA_Callback_State_t callback_state = I2S_DMA_CALLBACK_IDLE;
static FX_Delay_t dly_fx;

void processAudio(void)
{
    /*Counters for buffer pointer indexing*/
    int offset_r_ptr = 0;
    int offset_w_ptr = 0; 
    /*Pointer for writing processed samples to output buffer*/
    int w_ptr = 0;

    if (callback_state != I2S_DMA_CALLBACK_IDLE) {
        SEGGER_SYSVIEW_RecordVoid(33);
        SEGGER_SYSVIEW_PrintfHost("DSP: Processing started, callback state: %d", callback_state);
	    //decide if it was half or cplt callback
	    if (callback_state == I2S_DMA_CALLBACK_HALF)
        {
            //half callback, so we have to process the first half of the buffers
	    	offset_r_ptr = 0;
	    	offset_w_ptr = 0;
            //reset write pointer to the beginning of the float buffers
	    	w_ptr = 0;
	    }
	    else if (callback_state == I2S_DMA_CALLBACK_FULL)
        {
            //full callback, so we have to process the second half of the buffers
	    	offset_r_ptr = BLOCK_SIZE_U16;
	    	offset_w_ptr = BLOCK_SIZE_FLOAT;
            //set write pointer to the second half of the float buffers
	    	w_ptr = BLOCK_SIZE_FLOAT;
	    }

	    //restore input sample buffer to float array
	    for (int i=offset_r_ptr; i<offset_r_ptr+BLOCK_SIZE_U16; i=i+4)
        {
            //convert 16-bit samples to 32-bit float samples
	    	l_buf_in[w_ptr] = (float) ((int) (rxBuf[i]<<16)|rxBuf[i+1]);
	    	r_buf_in[w_ptr] = (float) ((int) (rxBuf[i+2]<<16)|rxBuf[i+3]);
	    	w_ptr++;
	    }

	    for (int i=offset_w_ptr; i<offset_w_ptr+BLOCK_SIZE_FLOAT; i++)
        {   
            //float temp_l_buff;
            //float temp_r_buff;
            //process input samples with selected effect
	    	l_buf_out[i] = (int) FX_Do_Delay(&dly_fx, l_buf_in[i]);
	    	r_buf_out[i] = (int) FX_Do_Delay(&dly_fx, r_buf_in[i]);
	    }

	    //restore processed float-array to output sample-buffer
	    w_ptr = offset_w_ptr;
	    for (int i=offset_r_ptr; i<offset_r_ptr+BLOCK_SIZE_U16; i=i+4) 
        {
            //convert 32-bit float samples to 16-bit samples
	    	txBuf[i] =  (((int)l_buf_out[w_ptr])>>16)&0xFFFF;
	    	txBuf[i+1] = ((int)l_buf_out[w_ptr])&0xFFFF;
	    	txBuf[i+2] = (((int)r_buf_out[w_ptr])>>16)&0xFFFF;
	    	txBuf[i+3] = ((int)r_buf_out[w_ptr])&0xFFFF;
	    	w_ptr++;
	    }
	    callback_state = I2S_DMA_CALLBACK_IDLE;
        SEGGER_SYSVIEW_Print("DSP: Processing finished");
        SEGGER_SYSVIEW_RecordEndCall(33);
    }
}

void audio_SetCallbackState(I2S_DMA_Callback_State_t state)
{
    callback_state = state;
}

void audio_InitFX(void)
{
    Reverb_Init();
    FX_Delay_Init(&dly_fx, 500, 0.3f, 0.5f); //500ms delay, 50% mix, 50% feedback
}

uint16_t* audio_getTxBuf(void)
{
    return txBuf;
}
uint16_t* audio_getRxBuf(void)
{
    return rxBuf;
}

