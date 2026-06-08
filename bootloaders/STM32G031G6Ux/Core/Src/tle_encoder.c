/*
 * tle_encoder.c
 *
 *  Created on: Jun 7, 2026
 *      Author: redux
 */
#include "dartt_map.h"
#include "trig_fixed.h"
#include "tle_encoder.h"

int32_t sin_mid = 2048;
int32_t cos_mid = 2048;
int32_t mul_cos = 1;
int32_t div_cos = 1;
int32_t mul_sin = 1;
int32_t div_sin = 1;
int32_t oerr_total_nsin = 0;
int32_t oerr_total_ncos = 1;

/*use the global encoder max and min params to claculate gains
 * and midpoints for commutation encoder.*/
void eval_encoder_params(void)
{
	sin_mid = (gl_dp.fds.sin_max + gl_dp.fds.sin_min)/2;
	cos_mid = (gl_dp.fds.cos_max + gl_dp.fds.cos_min)/2;
	mul_cos = (1<<14);
	div_cos = (gl_dp.fds.cos_max - gl_dp.fds.cos_min)/2;
	mul_sin = (1<<14);
	div_sin = (gl_dp.fds.sin_max - gl_dp.fds.sin_min)/2;
	oerr_total_nsin = sin_14b(-gl_dp.fds.oerr);
	oerr_total_ncos = cos_14b(-gl_dp.fds.oerr);
}


int32_t theta_rel_14b(void)
{
	int32_t sinVal = ((int32_t)(gl_dp.dma_adc_raw[ADC_SIN_CHAN]))-sin_mid;
	int32_t cosVal = ((int32_t)(gl_dp.dma_adc_raw[ADC_COS_CHAN]))-cos_mid;
	sinVal = (sinVal*mul_sin)/div_sin;
	cosVal = (cosVal*mul_cos)/div_cos;
	/*
	 * sin phase compensator term.
	 */
	sinVal = (sinVal*16384 - cosVal*oerr_total_nsin)/oerr_total_ncos;	//this is dangerous iff gl_oerr_total == PI_12B/2 (divide by 0 error). however this should be well outside the actual range of achievable values for this number
	return wrap_2pi_fixed(atan2_14b(sinVal,cosVal) - gl_dp.fds.offset, TWO_PI_14B);
}

