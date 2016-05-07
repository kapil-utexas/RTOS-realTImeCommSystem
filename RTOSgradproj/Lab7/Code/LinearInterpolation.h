#ifndef _LInter_H_
#define _LInter_H_


#include <stdint.h>
/*********Linear_Interpolation*********
 * Does linear interpolation on the given ADC value
 * returns the corresponding Temp value equal to it
 * input: uint32_t - ADC value
 * output: uint32_t - Temperature value 
 */
 
uint32_t Linear_Interpolation(uint32_t ADCvalue);

#endif
