/*
 * encoder-params.h
 *
 *  Created on: Jun 6, 2026
 *      Author: redux
 */

#ifndef INC_DARTT_MAP_H_
#define INC_DARTT_MAP_H_
#include "stdint.h"

#define NUM_ADC 2

typedef struct fds_t
{
	//dartt address
	uint32_t address;

	//for encoder calibration
	uint32_t sin_min;
	uint32_t sin_max;
	uint32_t cos_min;
	uint32_t cos_max;
	uint32_t oerr;

	//set zero angle
	int32_t offset;

	//uart settings
	uint32_t baud;
}fds_t;

typedef struct dartt_map_t
{
	int32_t angle;

	uint16_t dma_adc_raw[NUM_ADC];

	uint32_t tick;

	uint32_t action_flag;

	fds_t fds;
}dartt_map_t;

extern dartt_map_t gl_dp;

#endif /* INC_DARTT_MAP_H_ */
