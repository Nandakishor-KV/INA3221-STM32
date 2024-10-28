/*
 * INA3221.h
 *
 *  Created on: Oct 2, 2024
 *      Author: Nandakishor
 */

#ifndef INC_INA3221_H_
#define INC_INA3221_H_

extern float c3_i;
extern float c3_v;

void INA3221_configure(float);
void INA3221_read(void);
void calc_i(void);
void calc_v(void);
void oc_cutoff(void);

#include "stm32f4xx_hal.h"

#endif /* INC_INA3221_H_ */
