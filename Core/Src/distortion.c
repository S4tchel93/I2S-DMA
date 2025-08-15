#include "distortion.h"
#include <math.h>
#include "dsp_configuration.h"

float FX_OD_LPF_INP_COEF[FX_OD_LPF_INP_LENGTH] = { 0.0017537939320576863,0.007340938089113929,0.010684659857249475,
0.0059465192664006445,-0.0042426166630165525,-0.006997638595382362,0.002026157681616623,0.008949616068073098,0.0008912997218693121,
-0.010828116840199064,-0.005341942938835928,0.011629118328725593,0.011319672204048182,-0.010437864918854862,-0.01855507417411213,
0.006264099063997508,0.026534382918826812,0.002082360459837173,-0.034558207485434624,-0.016429660182211446,0.04186999401097673,
0.04105973748393176,-0.04773119656100209,-0.09174975610860517,0.051520616641375305,0.31338828169865396,0.4471688410545182,
0.31338828169865396,0.051520616641375305,-0.09174975610860517,-0.04773119656100209,0.04105973748393176,0.04186999401097673,
-0.016429660182211446,-0.034558207485434624,0.002082360459837173,0.026534382918826812,0.006264099063997508,-0.01855507417411213,
-0.010437864918854862,0.011319672204048182,0.011629118328725593,-0.005341942938835928,-0.010828116840199064,0.0008912997218693121,
0.008949616068073098,0.002026157681616623,-0.006997638595382362,-0.0042426166630165525,0.0059465192664006445,0.010684659857249475,
0.007340938089113929,0.0017537939320576863};

void FX_Overdrive_Init(FX_Overdrive_t* od, float hpfCutoffFrequencyHz, float odPreGain, float lpfCutoffFrequencyHz, float lpfDamping) {
	od->sampling_time = 1.0f / SAMPLE_RATE;

	// Input High pass filter
	od->hpfInpBufIn[0] = 0.0f;
	od->hpfInpBufIn[1] = 0.0f;
	od->hpfInpBufOut[0] = 0.0f;
	od->hpfInpBufOut[1] = 0.0f;
	od->hpfInpWcT = 2.0f * M_PI * hpfCutoffFrequencyHz * od->sampling_time;
	od->hpfInpOut = 0.0f;

	/*input low pass filter*/
	for(uint8_t i=0; i < FX_OD_LPF_INP_LENGTH; i++) {
		od->lpfInpBuf[i] = FX_OD_LPF_INP_COEF[i];
	}
	od->lpfInpBufIndex = 0;
	od->lpfInpOut = 0.0f;

	// Overdrive parameters
	od->preGain = odPreGain;   // Input gain
	od->threshold = 1.0f / 3.0f; // Clipping threshold

	/*output low pass filter*/
	od->lpfOutWcT = 2.0f * M_PI * lpfCutoffFrequencyHz * od->sampling_time;
	od->lpfOutDamp = lpfDamping;
}

void FX_Overdrive_SetHPF(FX_Overdrive_t* od, float hpfcutoffFrequencyHz) {
	od->hpfInpWcT = 2.0f * M_PI * hpfcutoffFrequencyHz * od->sampling_time;
}

void FX_Overdrive_SetLPF(FX_Overdrive_t* od, float lpfCutoffFrequencyHz, float lpfDamping) {
	od->lpfOutWcT = 2.0f * M_PI * lpfCutoffFrequencyHz * od->sampling_time;
	od->lpfOutDamp = lpfDamping;
}

float FX_Do_Overdrive(FX_Overdrive_t* od, float inSample) {
	// Low Pass filter anti aliasing
	od->lpfInpBuf[od->lpfInpBufIndex] = inSample;
	od->lpfInpBufIndex++;

	if(od->lpfInpBufIndex >= FX_OD_LPF_INP_LENGTH) {
		od->lpfInpBufIndex = 0;
	}
	
	od->lpfInpOut = 0.0f;
	uint8_t bufIndex = od->lpfInpBufIndex;
	for(uint8_t i=0; i < FX_OD_LPF_INP_LENGTH; i++) {
		if(bufIndex == 0) {
			bufIndex = FX_OD_LPF_INP_LENGTH - 1;
		}
		else {
			bufIndex++;
		}
		od->lpfInpOut += od->lpfInpBuf[i] * FX_OD_LPF_INP_COEF[i];
	}

	/*Variable first order IIR High Pass filter to remove some of the low frequency components*/
	od->hpfInpBufIn[1] = od->hpfInpBufIn[0];
	od->hpfInpBufIn[0] = od->lpfInpOut;

	od->hpfInpBufOut[1] = od->hpfInpBufOut[0];
	od->hpfInpBufOut[0] = (2.0f*(od->hpfInpBufIn[0] - od->hpfInpBufIn[1]) + (2.0f - od->hpfInpWcT)*od->hpfInpBufOut[1]) / (2.0f + od->hpfInpWcT);
	od->hpfInpOut = od->hpfInpBufOut[0];

	/*overdrive*/
	float clipIn = od->preGain * od->hpfInpOut;
	float absClipIn = fabs(clipIn);
	float signClipIn = (clipIn > 0.0f) ? 1.0f : -1.0f;

	float clipOut = 0.0f;

	if(absClipIn < od->threshold) {
		clipOut = 2.0f * clipIn;
	}
	else if(absClipIn >= od->threshold && absClipIn < (2.0f * od->threshold)) {
		clipOut = signClipIn * (3.0f - (2.0f - 3.0f * absClipIn) * (2.0f - 3.0f * absClipIn)) / 3.0f;
	}
	else {
		clipOut = signClipIn;
	}

	/*Variable 2nd order IIR low pass filter to remove high frequency components after clipping*/
	od->lpfOutBufIn[2] = od->lpfOutBufIn[1];
	od->lpfOutBufIn[1] = od->lpfOutBufIn[0];
	od->lpfOutBufIn[0] = clipOut;

	od->lpfOutBufOut[2] = od->lpfOutBufOut[1];
	od->lpfOutBufOut[1] = od->lpfOutBufOut[0];
	od->lpfOutBufOut[0] = (od->lpfOutWcT * od->lpfOutWcT * (od->lpfOutBufIn[0] + 2.0f * od->lpfOutBufIn[1] + od->lpfOutBufIn[2])
			- 2.0f * (od->lpfOutWcT * od->lpfOutWcT -4.0f) * od->lpfOutBufOut[1]
			- (4.0f - 4.0f * od->lpfOutDamp * od->lpfOutWcT + od->lpfOutWcT * od->lpfOutWcT) * od->lpfOutBufOut[2]);
	od->lpfOutBufOut[0] /= (4.0f + 4.0f * od->lpfOutDamp * od->lpfOutWcT + od->lpfOutWcT * od->lpfOutWcT);
	od->lpfOutOut = od->lpfOutBufOut[0];

	/*limit output*/
	od->out = od->lpfOutOut;

	//if (od->out > 1.0f) {
	//	od->out = 1.0f;
	//}
	//else if (od->out < -1.0f) {
	//	od->out = -1.0f;
	//}

	return od->out;
}