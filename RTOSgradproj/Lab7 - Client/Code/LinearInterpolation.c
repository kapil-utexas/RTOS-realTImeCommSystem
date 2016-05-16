#include "LinearInterpolation.h"
#include "IRData.h"
#include <stdint.h>


/*********binarySearch**********
 * Binary Search on ADCdata looking for spot of ADCvalue
 * input: uint32_t ADC value
 * output: uint32_t index - of the value directly less than the given ADC
 */
static uint32_t binarySearch(uint32_t ADCvalue){
	uint32_t high = ADC_SIZE - 1;
	uint32_t low = 0;
	while(low < high){
	  uint32_t mid = (high + low)/2; 
		uint32_t ADCTabVal = ADCdata[mid];
		if(ADCTabVal == ADCvalue){								// Directly found the ADCvalue
			return mid;
		}
		else if(ADCvalue < ADCTabVal){						// look at lower half
			high = mid - 1;
		}
		else{																			// look at upper half
			low = mid + 1;
		}
	}
	if(ADCdata[low] < ADCvalue)	
		return low;
	else
		return low - 1;
}

/*********Linear_Interpolation*********
 * Does linear interpolation on the given ADC value
 * returns the corresponding Temp value equal to it
 * input: uint32_t - ADC value
 * output: uint32_t - Temperature value 
 */
 
uint32_t Linear_Interpolation(uint32_t ADCvalue){
	if(ADCvalue<=40)
		ADCvalue = 40;
	else if(ADCvalue>=305)
		ADCvalue=305;
	uint32_t index = binarySearch(ADCvalue);
	// will put the negative later 
	uint32_t tempDif = IRdata[index] - IRdata[index + 1];		// solving for slope
	uint32_t ADCDif = ADCdata[index + 1] - ADCdata[index];
	// (y - y1) = m(x - x1)    
	// y is the Temp and x is the ADCvalue
	uint32_t b = tempDif * ADCdata[index] / ADCDif;
	b = IRdata[index] - b;
	uint32_t y = tempDif * ADCvalue / ADCDif + b;
	return y;
}


