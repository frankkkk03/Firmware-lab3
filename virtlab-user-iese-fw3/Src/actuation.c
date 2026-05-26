/*
 * actuation.c
 *
 *  Created on: May 26, 2026
 *      Author: franc
 *
 *  Actuation task for Firmware-LAB4
 *      temperature < TARGET -> heater ON (GPIO high, transistor saturated)
 *      temperature >= TARGET -> heater OFF (GPIO low, transistor off)
 *
 */

#include "main.h"
/* Provides the  HEATER_CTRL_Pin e HEATER_CTRL_GPIO_Port macros
 * and the HAL_GPIO_WritePin function */
#include "cmsis_os2.h"
/*Provides osMessageQueueGet(), the osMessageQueueId_t and osStatus_t types,
 * the osWaitForever constant and the osOk return value*/
#include <stdint.h>
/*Provides the uint16_t type used for the temperature value*/

/*Input queue, created in main.c*/
extern osMessageQueueId_t actuationQueueHandle;

/*Target temperature in tenths of Kelvin, modified from 50°C to approximately 37°C*/
#define TARGET_TEMP_X10		3105

/*Actuation task entry point*/
void StartActuationTask(void *argument){

	(void)argument;	//silence the compiler warning

	/*Force the heater OFF before entering the loop
	 * The pin is already initialised low by CubeMX but in this way is more explicit*/
	HAL_GPIO_WritePin(HEATER_CTRL_GPIO_Port, HEATER_CTRL_Pin, GPIO_PIN_RESET);
	for( ; ; ) {	//infinite loop, as required for an RTOS task

		uint16_t temperature;

		/*Read one value form the queue into 'temperature' variable. It
		 * blocks until a value is available*/
		osStatus_t status = osMessageQueueGet(actuationQueueHandle, &temperature, NULL, osWaitForever);
		if(status==osOK){
			if(temperature<TARGET_TEMP_X10){
				/*turn the heater ON*/
				HAL_GPIO_WritePin(HEATER_CTRL_GPIO_Port, HEATER_CTRL_Pin, GPIO_PIN_SET);
			}
			else {
				/*turn the heater OFF*/
				HAL_GPIO_WritePin(HEATER_CTRL_GPIO_Port, HEATER_CTRL_Pin, GPIO_PIN_RESET);
			}
		}
		//GPIO_PIN_RESET = 0V, GPIO_PIN_SET = 3.3V
	}

}


