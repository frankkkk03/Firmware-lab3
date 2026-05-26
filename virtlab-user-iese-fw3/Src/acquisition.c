/*
 * acquisition.c
 *
 *  Created on: May 18, 2026
 *      Author: max
 *
 * Acquisition task for Firmware-LAB3
 *
 * Waits for the periodic event flag set by tim1Callback, reads ADC1 (NTC divider) and
 * AND2 (Vcc/2 reference divider), computes the NTC temperature in tenths of Kelvin in order to avoid float
 * computation that may cause errors because of the sistematic errors, only using integer arithmetic
 * and pushes the result to acquisitionQueue
 */

#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>

//Output queue, defined in main.c
extern osMessageQueueId_t acquisitionQueueHandle;

// ADC handles, defined in main.c
extern ADC_HandleTypeDef hadc1;	//reads AIN3, the NTC divider
extern ADC_HandleTypeDef hadc2; //reads AIN4, the fixed 10k/10k divider

/*Event flags object, created here and used by application.c*/
osEventFlagsId_t acqEventFlagsHandle;

/*Preprocessor constant*/
#define ACQ_FLAG_START (1U << 0)	//1

/*Physical constants*/
#define R0_NTC		10000	//NTC nominal resistance at 25 C (in ohm)
#define KELVIN_X10  2731	// 273.1 * 10, offset Celsius to Kelvin
#define ADC_TIMEOUT_MS 10	//maximum wait for an ADC conversion
#define ADC_MAX_COUNT ((1U << 12) - 1U) //(2^12-1)=4095
#define NTC_COEFF_INV_X10 250 // (1/0.04) *10


/*Acquisition task entry point
 * Every RTOS task takes a generic pointer as its parameter*/
void StartAcquisitionTask( void *argument ) {

	(void)argument;  /* unused, silence compiler warning */

	/*Create the event flags object (it must be done after kernel starts)*/
	acqEventFlagsHandle = osEventFlagsNew(NULL);

  /* Infinite loop: an RTOS task must never return*/
  for( ; ; ) {

	/*Wait for the periodic event set by tim1Callback*/
    osEventFlagsWait(acqEventFlagsHandle, ACQ_FLAG_START, osFlagsWaitAny, osWaitForever);

    //ADC1 reading
    if( HAL_ADC_Start(&hadc1) != HAL_OK){
    	continue;
    }
    HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT_MS);
    uint32_t adc1 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    //ADC2 reading
    if( HAL_ADC_Start(&hadc2) != HAL_OK){
        	continue;
        }
  	HAL_ADC_PollForConversion(&hadc2, ADC_TIMEOUT_MS);
    uint32_t adc2 = HAL_ADC_GetValue(&hadc2);
    HAL_ADC_Stop(&hadc2);

    /*Compute the denominator (2*ADC2-ADC1)*/
    int32_t denom = (int32_t)(2u * adc2) - (int32_t)adc1;

    if (denom <= 0)		//protection against an open or shorted circuit input
    	continue;

    /*Compute NTC resistance: R_NTC = R0 * ADC1 / (2*ADC2-ADC1) */
    int32_t r_ntc = (int32_t)R0_NTC * (int32_t)adc1 / denom;

    /*Compute temperature in tenths of Celsius:
     * T_Celsius = T_ref + (R0 - R_NTC) * (1/coeff) / R0, all scaled by ten*/
    int32_t t_celsius_x10 = 250 + ((int32_t)R0_NTC - r_ntc) * NTC_COEFF_INV_X10 / (int32_t)R0_NTC;

    /*Convert to tenths of Kelvin*/
    int32_t t_kelvin_x10 = t_celsius_x10 + KELVIN_X10;

    /*Range check before casting to uint16_t*/
    if(t_kelvin_x10 <= 0 || t_kelvin_x10 > 65535) // 2^16-1 = 65535
    	continue;
    uint16_t t_encoded = (uint16_t)t_kelvin_x10;

    /* Push the result to the acquisition queue */
    osMessageQueuePut(acquisitionQueueHandle, &t_encoded, 0U, 0U);

  }
}

