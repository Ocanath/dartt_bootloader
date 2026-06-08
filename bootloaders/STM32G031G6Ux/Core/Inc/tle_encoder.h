/*
 * tle_encoder.h
 *
 *  Created on: Jun 7, 2026
 *      Author: redux
 */

#ifndef INC_TLE_ENCODER_H_
#define INC_TLE_ENCODER_H_
#include "trig_fixed.h"

#define ADC_COS_CHAN 0
#define ADC_SIN_CHAN 1

void eval_encoder_params(void);
int32_t theta_rel_14b(void);


#endif /* INC_TLE_ENCODER_H_ */
