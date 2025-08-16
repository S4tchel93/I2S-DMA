#ifndef DSP_CFG_H
#define DSP_CFG_H

/*Main Audio engine settings*/
#define BLOCK_SIZE_U16 128
#define BLOCK_SIZE_FLOAT BLOCK_SIZE_U16 / 4
#define SAMPLE_RATE 48000

/*Effects compile settings*/
#define REVERB_ENABLE
#define DELAY_ENABLE
#define OVERDRIVE_ENABLE

#define OD_GAIN_MIN 1.0f
#define OD_GAIN_SCALE 50.0f
#define OD_GAIN_BOOST 50.0f

#define OD_HPF_CUTOFF_MAX 2000.0f
#define OD_HPF_CUTOFF_MIN 1000.0f

#define OD_LPF_CUTOFF_MAX 2500.0f
#define OD_LPF_CUTOFF_SCALE 7500.0f
#define OD_LPF_DAMP 1.0f

#endif // DSP_CFG_H