/*
 * acquisition.c
 *
 *  Created on: May 18, 2026
 *      Author: max
 */

#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>

// Declaration of output queue, defined in main.c

//la parola extern significa che la variabile è definita in un altro file .c

extern osMessageQueueId_t acquisitionQueueHandle;

//
extern ADC_HandleTypeDef hadc1;	//legge il pin AIN3 connesso al partitore del NTC
extern ADC_HandleTypeDef hadc2; //legge il pin AIN4 connesso al partitore fisso 10k/10k

osEventFlagsId_t acqEventFlagsHandle;

//costante x preprocessore
#define ACQ_FLAG_START (1U << 0)	//rappresenta 1 con suffisso unsigned
// (<<0) rappresenta kl'operazione di shift a sinistra
//costanti fisiche
#define R0_NTC		10000	//resistenza nominale dell'NTC a 25°C
#define KELVIN_X10  2731	// moltiplicato x10 perchèp lavoriamo in decimi di kelvin
#define ADC_TIMEOUT_MS 10	//timeout
#define ADC_MAX_COUNT ((1U << 12) - 1U) //(2^12-1)=4095
#define NTC_COEFF_INV_X10 250 // (1/0.04) *10


// Acquisition task entry point

//tutti i task RTOS devono ricevere un puntatore generico come parametro
void StartAcquisitionTask( void *argument ) {
	acqEventFlagsHandle = osEventFlagsNew(NULL);
	//crea un nuovo oggetto eventsflag
  /* Infinite loop */
/*tutti i task RTOS devono essere loop infiniti, se un task ritornasse,
 * l'RTOS andrebbe in errore*/
  for( ; ; ) {
    //osDelay( 1 );		??
	  (void)argument;  /* unused, silence compiler warning */

    osEventFlagsWait(acqEventFlagsHandle, ACQ_FLAG_START, osFlagsWaitAny, osWaitForever);

    //lettura di ADC1
    if( HAL_ADC_Start(&hadc1) != HAL_OK){
    	continue;
    }
    /*if( HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT_MS) != HAL_OK) {
    	HAL_ADC_Stop(&hadc1);
    	continue;*/
    HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT_MS);
    uint32_t adc1 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    //lettura di ADC2
    if( HAL_ADC_Start(&hadc2) != HAL_OK){
        	continue;
        }
        /*if( HAL_ADC_PollForConversion(&hadc2, ADC_TIMEOUT_MS) != HAL_OK) {
        	HAL_ADC_Stop(&hadc2);
        	continue;*/
  	  HAL_ADC_PollForConversion(&hadc2, ADC_TIMEOUT_MS);
        uint32_t adc2 = HAL_ADC_GetValue(&hadc2);
        HAL_ADC_Stop(&hadc2);

    //Calcolo del denominatore
    int32_t denom = (int32_t)(2u * adc2) - (int32_t)adc1;
    //protezione da circuito aperto o cortocircuito
    if (denom <= 0)
    	continue;

    //Calcolo di NTC
    int32_t r_ntc = (int32_t)R0_NTC * (int32_t)adc1 / denom;

    //calcolo temperatura in decimi di Celsius
    int32_t t_celsius_x10 = 250 + ((int32_t)R0_NTC - r_ntc) * NTC_COEFF_INV_X10 / (int32_t)R0_NTC;
    //Tref + (R0-R_NTC)*inverso_coefficiente / R0 tutto scalato x10
    //conversione in decimi di kelvin
    int32_t t_kelvin_x10 = t_celsius_x10 + KELVIN_X10;

    //check del valore
    if(t_kelvin_x10 <= 0 || t_kelvin_x10 > 65535)
    	continue;
    uint16_t t_encoded = (uint16_t)t_kelvin_x10;

    //invio nella coda
    osMessageQueuePut(acquisitionQueueHandle, &t_encoded, 0U, 0U);

  }
}

