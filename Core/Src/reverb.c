#include "reverb.h"
#include "dsp_configuration.h"

#ifdef REVERB_ENABLE
//Schroeder delays from 25k->96k interpolated
//*2 delay extension -> not more possible without external ram
#define l_CB0 3460*2
#define l_CB1 2988*2
#define l_CB2 3882*2
#define l_CB3 4312*2
#define l_AP0 480*2
#define l_AP1 161*2
#define l_AP2 46*2

//define wet 0.0 <-> 1.0
static float wet = 0.25f;
//define time delay 0.0 <-> 1.0 (max)
static float time = 0.7f;

//define pointer limits = delay time
static int cf0_lim, cf1_lim, cf2_lim, cf3_lim, ap0_lim, ap1_lim, ap2_lim;
//define buffer for comb- and allpassfilters
static float  cfbuf1[l_CB1], cfbuf2[l_CB2], apbuf0[l_AP0];
/*Send these to CCM RAM to make more ram space to include other effects on normal RAM*/
__attribute__((section(".ccmram")))static float cfbuf0[l_CB0] = {}, cfbuf3[l_CB3] = {}, apbuf2[l_AP2] = {}, apbuf1[l_AP1] = {};
//feedback defines as of Schroeder
static float cf0_g = 0.805f, cf1_g=0.827f, cf2_g=0.783f, cf3_g=0.764f;
static float ap0_g = 0.7f, ap1_g = 0.7f, ap2_g = 0.7f;
//buffer-pointer
static int cf0_p=0, cf1_p=0, cf2_p=0, cf3_p=0, ap0_p=0, ap1_p=0, ap2_p=0;

static float Do_Comb0(float inSample);
static float Do_Comb1(float inSample);
static float Do_Comb2(float inSample);
static float Do_Comb3(float inSample);
static float Do_Allpass0(float inSample);
static float Do_Allpass1(float inSample);
static float Do_Allpass2(float inSample);
static float Process_Reverb(float inSample);

static float Do_Comb0(float inSample) {
	float readback = cfbuf0[cf0_p];
	float new = readback*cf0_g + inSample;
	cfbuf0[cf0_p] = new;
	cf0_p++;
	if (cf0_p==cf0_lim) cf0_p = 0;
	return readback;
}

static float Do_Comb1(float inSample) {
	float readback = cfbuf1[cf1_p];
	float new = readback*cf1_g + inSample;
	cfbuf1[cf1_p] = new;
	cf1_p++;
	if (cf1_p==cf1_lim) cf1_p = 0;
	return readback;
}

static float Do_Comb2(float inSample) {
	float readback = cfbuf2[cf2_p];
	float new = readback*cf2_g + inSample;
	cfbuf2[cf2_p] = new;
	cf2_p++;
	if (cf2_p==cf2_lim) cf2_p = 0;
	return readback;
}

static float Do_Comb3(float inSample) {
	float readback = cfbuf3[cf3_p];
	float new = readback*cf3_g + inSample;
	cfbuf3[cf3_p] = new;
	cf3_p++;
	if (cf3_p==cf3_lim) cf3_p = 0;
	return readback;
}


static float Do_Allpass0(float inSample) {
	float readback = apbuf0[ap0_p];
	readback += (-ap0_g) * inSample;
	float new = readback*ap0_g + inSample;
	apbuf0[ap0_p] = new;
	ap0_p++;
	if (ap0_p == ap0_lim) ap0_p=0;
	return readback;
}

static float Do_Allpass1(float inSample) {
	float readback = apbuf1[ap1_p];
	readback += (-ap1_g) * inSample;
	float new = readback*ap1_g + inSample;
	apbuf1[ap1_p] = new;
	ap1_p++;
	if (ap1_p == ap1_lim) ap1_p=0;
	return readback;
}

static float Do_Allpass2(float inSample) {
	float readback = apbuf2[ap2_p];
	readback += (-ap2_g) * inSample;
	float new = readback*ap2_g + inSample;
	apbuf2[ap2_p] = new;
	ap2_p++;
	if (ap2_p == ap2_lim) ap2_p=0;
	return readback;
}

static float Process_Reverb(float inSample) {
	float newsample = (Do_Comb0(inSample) + Do_Comb1(inSample) + Do_Comb2(inSample) + Do_Comb3(inSample))/4.0f;
	newsample = Do_Allpass0(newsample);
	newsample = Do_Allpass1(newsample);
	newsample = Do_Allpass2(newsample);
	return newsample;
}
#endif // REVERB_ENABLE

float Do_Reverb(float inSample) {
    float sum = inSample;
#ifdef REVERB_ENABLE
	sum = (1.0f-wet)*sum + wet*Process_Reverb(sum);
#endif // REVERB_ENABLE

    return sum;
}

void Reverb_Init(void)
{
#ifdef REVERB_ENABLE
    cf0_lim = (int)(time*l_CB0);
	cf1_lim = (int)(time*l_CB1);
	cf2_lim = (int)(time*l_CB2);
	cf3_lim = (int)(time*l_CB3);
	ap0_lim = (int)(time*l_AP0);
	ap1_lim = (int)(time*l_AP1);
	ap2_lim = (int)(time*l_AP2);
#endif // REVERB_ENABLE
}