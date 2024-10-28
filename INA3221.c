/*
 * INA3221.c
 *
 *  Created on: Oct 2, 2024
 *      Author: Nandakishor
 */

#include "INA3221.h"

extern I2C_HandleTypeDef hi2c1;  // change your handler here accordingly

#define Relay_Signal_Pin GPIO_PIN_12 // Change pin and port for relay signal accordingly
#define Relay_Signal_GPIO_Port GPIOB

static const uint8_t SLAVE_ADDRESS_INA3221 = 0x40 << 1; // change this according to ur setup

static const uint8_t REG_Config = 0x00;
static const uint8_t REG_C3_ShuntV = 0x05;
static const uint8_t REG_C3_BusV = 0x06;

uint8_t mycfg[2] = { 0x75, 0xFF }; //Average = 16 samples, Conversion Time = 8.244ms (Refer Datasheet Pg No. 26)
uint8_t buf[2];
uint8_t buf_v[2];
uint8_t buf_i[2];
HAL_StatusTypeDef ret = 0;
int16_t config_out = 0;
int16_t c3_sv_reg_val = 0;
int16_t c3_bv_reg_val = 0;
float c3_sv_magnitude = 0;
float c3_i = 0;
float c3_v = 0;
float oc_threshold = 0.5; //Threshold current for overcurrent protection

void INA3221_configure(float oc_threshold_custom)
{
	ret = HAL_I2C_Mem_Write(&hi2c1, SLAVE_ADDRESS_INA3221, REG_Config,
			I2C_MEMADD_SIZE_8BIT, mycfg, 2, HAL_MAX_DELAY);

	if (ret == HAL_OK) {

		ret = HAL_I2C_Mem_Read(&hi2c1, SLAVE_ADDRESS_INA3221, REG_Config,
				I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);

		if (ret == HAL_OK) {
			config_out = ((uint16_t) buf[0] << 8) | buf[1];
		}
	}

	if (config_out != 0x75FF) {
		return 0;
	}

	HAL_GPIO_WritePin(Relay_Signal_GPIO_Port, Relay_Signal_Pin, 1);

	oc_threshold = oc_threshold_custom;

	HAL_I2C_Mem_Read_DMA(&hi2c1, SLAVE_ADDRESS_INA3221, REG_C3_ShuntV,
				I2C_MEMADD_SIZE_8BIT, buf_i, 2);
}

void INA3221_read(void)
{

	HAL_I2C_Mem_Read_DMA(&hi2c1, SLAVE_ADDRESS_INA3221, REG_C3_ShuntV,
				I2C_MEMADD_SIZE_8BIT, buf_i, 2);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Memaddress == REG_C3_ShuntV) {
		calc_i();
	} else if (hi2c->Memaddress == REG_C3_BusV) {
		calc_v();
	}
}

void calc_i() {
	c3_sv_reg_val = ((uint16_t) buf_i[0] << 8) | buf_i[1];
	c3_sv_reg_val = (c3_sv_reg_val >> 3) & 0x1FFF;
	if (c3_sv_reg_val & 0x1000) {
		c3_sv_reg_val = -((~c3_sv_reg_val + 1) & 0x1FFF);
	}
	c3_sv_magnitude = c3_sv_reg_val * 40e-6f;

	c3_i = c3_sv_magnitude / 0.1f; //0.1 ohm R

	oc_cutoff();
}

void calc_v() {
	c3_bv_reg_val = ((uint16_t) buf_v[0] << 8) | buf_v[1];
	c3_bv_reg_val = (c3_bv_reg_val >> 3) & 0x1FFF;

	c3_v = c3_bv_reg_val * 0.008;

	HAL_I2C_Mem_Read_DMA(&hi2c1, SLAVE_ADDRESS_INA3221, REG_C3_ShuntV,
			I2C_MEMADD_SIZE_8BIT, buf_i, 2);
}

void oc_cutoff(){
	if(c3_i >= oc_threshold){
		HAL_GPIO_WritePin(Relay_Signal_GPIO_Port, Relay_Signal_Pin, 0);
	}

	HAL_I2C_Mem_Read_DMA(&hi2c1, SLAVE_ADDRESS_INA3221, REG_C3_BusV,
			I2C_MEMADD_SIZE_8BIT, buf_v, 2);
}

