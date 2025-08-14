#include "distortion.h"
#include <math.h>

float Do_Distortion (float insample) {
	float threshold_noise = 2000000.0f;
	float threshold_lower = 10000000.0f;
	float gain_lower = 2.0f;

	float threshold_higher = 60000000.0f;
	float gain_higher = 0.5f;

	float outgain = 2.0f;

	if (fabs(insample) < threshold_lower && fabs(insample) > threshold_noise ) return outgain*(insample*gain_lower);
	if (fabs(insample) > threshold_higher) return outgain*(insample*gain_higher);
	return outgain*insample;
}