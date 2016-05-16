// ADC.h
// Runs on LM4F120/TM4C123
// Provide a function that initializes Timer0A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Allen Wang and Alvin Tung
// 23 Jan 2016

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
 #ifndef _ADC_H_
 #define _ADC_H_
 
 #include <stdint.h>
 
#define T0A_TIMER_BITS       16        
#define T0A_PRESCALER_BITS   4                                      // bits added to 16 for timer
#define T0A_PRESCALER        ((1 << T0A_PRESCALER_BITS)-1)          // value to be written in TAPR
#define T0A_TOTAL_TIMER_BITS (T0A_TIMER_BITS + T0A_PRESCALER_BITS)  // extra clock cycles per 1 timer clock

#define ADC_MIN_FREQ         100                                    // (SYSCLK / (1 << T0A_TOTAL_TIMER_BITS)) 
#define ADC_MAX_FREQ         10000                                  //  16000000 // Max ADC frequency (see reference manual)

//---------------ADC_Init---------------
// Initialization function for T0ATrigger
// Ouput: none
// Input: period between ADC conversions and
//        ADC channel number
// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer0A: enabled
// Mode: 16-bit, down counting
// One-shot or periodic: periodic
// Interval value: programmable using 32-bit period
// Sample time is busPeriod*period
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// SS3 1st sample source: programmable using variable 'channelNum' [0:11]
// SS3 interrupts: enabled and promoted to controller
void ADC_Init(uint8_t channelNum);

//---------------ADC_Open---------------
// Changes current ADC channel
// Ouput: none
// Input: unsigned int, 'channelNum' [0:11]
void ADC_Open(unsigned int channelNum);

//---------------ADC_In---------------
// Busy-wait captures ADC single sample **(maybe use interrupt?)
// Ouput: 16 bit ADC data value 
// Input: none
unsigned short ADC_In(void);

//---------------ADC_Status--------------
// Status of ADC if collection is complete
// Ouput: 0 for complete, 1 for in progress 
// Input: none
uint8_t ADC_Status(void);

//---------------ADC_Collect---------------
// Uses T0A interrupt to collect ADC samples
// Ouput: ADC_Status
// Input: The channel, frequency, and size to 
//        sample and where to store values 
int ADC_Collect(unsigned int channelNum, unsigned int fs, 
  void(*task)(unsigned long));

//---------------ADC_ConvertToVolts--------------
// Converts ADC sample to Volts
// Ouput: decimal fixed point value for volts
// Input: ADC conversion value
uint16_t ADC_ConvertToVolts(uint16_t value);

#endif /* _ADC_H_ */
