/*
 * acquisition.c
 *
 *  Created on: May 18, 2026
 *      Author: max
 *
 * Display task for Firmware-LAB3
 * Waits for a temperature value (in tenths of Kelvin) from displayQueue and
 * writes it on the 4-digit LCD of the VirtLAB board */

#include "main.h"
#include "cmsis_os.h"
#include "io.h"
#include <stdint.h>

/*Input queue, defined in main.c*/
extern osMessageQueueId_t displayQueueHandle;

/* Function used to a 4-digit temperature value on the LCD.
 * The value is in tenths of Kelvin, without the decimal dot*/
static void lcdWriteTemperature (uint16_t t_tenths_kelvin){

	/*Spit the value into 4 decimal digits: d0 = hundreds, d1 = tens, d2 = units, d3 = tenths*/
	uint8_t d3 = t_tenths_kelvin % 10;
	uint8_t d2 = ( t_tenths_kelvin / 10 ) % 10;
	uint8_t d1 = ( t_tenths_kelvin / 100 ) % 10;
	uint8_t d0 = ( t_tenths_kelvin / 1000 ) % 10;

	/* Convert each digit to its ASCII character by adding '0' char and write it at the
	 * corresponding LCD position*/
	lcdWriteDigit('0' + d0, 0);
	lcdWriteDigit('0' + d1, 1);
	lcdWriteDigit('0' + d2, 2);
	lcdWriteDigit('0' + d3, 3);

	/*Commit the frame buffer to the physical display: without using this function, the display
	 * will never show any value*/
	lcdUpdateDisplay();
}

/*Display task enrty point*/
void StartDisplayTask( void *argument ) {
	(void)argument;

  /* Infinite loop */
  for( ; ; ) {
	  uint16_t temperature;

	  /*Block until a value is available in displayQueue */
	  osStatus_t status = osMessageQueueGet (displayQueueHandle, &temperature, NULL, osWaitForever);
	  if(status==osOK){
		 lcdWriteTemperature(temperature);
	  }
  }
}

