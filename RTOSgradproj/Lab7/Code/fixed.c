// ******** fixed.c ************** 
// Alvin Tung
// January 23, 2015
// Lab 01
// TA:
// January 31, 2015

#include <stdint.h>
#include <stdio.h>
#include "UART.h"

#define UMAX 10000				// Maximum unsigned value to divide with
#define SMAX 1000					// Maximum signed value to divide with
#define MIN  0						// Minimum value to compare with
#define ASCI 48						// ASCII offset

/****************Fixed_uDecOut2s***************
 converts fixed point number to ASCII string
 format unsigned 32-bit with resolution 0.01
 range 0 to 999.99
 Input: unsigned 32-bit integer part of fixed point number
         greater than 99999 means invalid fixed-point number
 Output: null-terminated string exactly 6 characters plus null
 Examples
 12345 to "123.45"  
 22100 to "221.00"
   102 to "  1.02" 
    31 to "  0.31" 
100000 to "***.**"    */ 
void Fixed_uDecOut2s(uint32_t n,  char *string){
  uint32_t div = UMAX;										// divisor to pull out digits
  uint32_t hold;													// hold digit value
  char val = 0;														// boolean if value else no value
  char k = 0;															// counter for loops
  
  /*CHECK FOR ERROR*/
  if (n >= UMAX * 10 ) {									// checks if greater than 100,000
    for(k = 0; k < 7; k++){								// creates string "***.**" with 0 terminating
      if ( k == 3 ){
        string[k] = '.';
      } 
      else if(k == 6){
        string[k] = 0;
      }
      else {
        string[k] = '*';
      }
    }
  } 
  /*NO ERROR*/
  else {
    for(k = 0; k < 6; k++) {							// creates fixed-point output string
      if( k == 3) {
        string[k] = '.';									// place decimal in the same spot
      }
      else{
        hold = n / div;										// grabs the digits value
        if(!hold && k < 2 && !val) {			// checks if 0 before any value greater than ten's digit 
          string[k] = ' ';								// if so add space
        } 
        else{
          string[k] = hold + ASCI;				// else add digits value 
          val = 1;
        }
        n -= (hold * div);								// subtracting that value from overall
        div /= 10;												// divide divisor by ten to check the next place
      }
    }
    string[k] = 0;												// adds terminating 0
  }
}

 /****************Fixed_uDecOut2***************
 outputs the fixed-point value on the display
 format unsigned 32-bit with resolution 0.01
 range 0 to 999.99
 Input: unsigned 32-bit integer part of fixed point number
         greater than 99999 means invalid fixed-point number
 Output: none
 Examples
 12345 to "123.45"  
 22100 to "221.00"
   102 to "  1.02" 
    31 to "  0.31" 
100000 to "***.**"    */ 
void Fixed_uDecOut2(uint32_t n){
  char string[7];													// string holder
  Fixed_uDecOut2s(n, string);							// use previous function to create string
  printf("%s", string);
}

 /****************Fixed_uDecOut3***************
 outputs the fixed-point value on the display
 format unsigned 32-bit with resolution 0.001
 range 0 to 99.999
 Input: unsigned 32-bit integer part of fixed point number
         greater than 99999 means invalid fixed-point number
 Output: none
 Examples
 12345 to "12.345"  
 22100 to "22.100"
   102 to " 0.102" 
    31 to " 0.031" 
100000 to "**.***"    */ 
void Fixed_uDecOut3(uint32_t n){
  char string[7];													// string holder
  char holder;														
  Fixed_uDecOut2s(n, string);							// use previous function to create string
  holder = string[2];											// switch decimal and value to create desired output
  string[2] = string[3];
  string[3] = holder;
  if(string[0] == ' ' && string[1] == ' '){
    string[1] = '0';
  }
  printf("%s", string);
}

/****************Fixed_sDecOut3s***************
 converts fixed point number to ASCII string
 format signed 32-bit with resolution 0.001
 range -9.999 to +9.999
 Input: signed 32-bit integer part of fixed point number
 Output: null-terminated string exactly 6 characters plus null
 Examples
  2345 to " 2.345"  
 -8100 to "-8.100"
  -102 to "-0.102" 
    31 to " 0.031" 
   
 */ 
void Fixed_sDecOut3s(int32_t n, char *string){
  uint32_t div = SMAX;
  uint32_t hold;
  char k = 0;
  char neg = 0;									// checker to see if negative
  char val = 0;									// checker to not display unnecessary 0's 
  
  /*CHECK FOR NEGATIVE*/
  if ( n < 0 ){
    neg = 1;
    n *= -1;
  }
	/*CHECK FOR ERROR*/
  if (n >= SMAX * 10 ) {
    for(k = 0; k < 7; k++){
      if ( k == 0 ){
        string[k] = ' ';
      }
      else if ( k == 2 ){
        string[k] = '.';
      } 
      else if ( k == 6 ){
        string[k] = 0;
      } 
      else {
        string[k] = '*';
      }
    }
  } 
  /*NO ERROR*/
  else {
    for(k = 0; k < 6; k++){
      if( k == 0 && neg) {							// places negative in front if negative
        string[k] = '-';
      }
      else if ( k == 0 && !neg){				// see previous function for clarification
        string[k] =' ';
      }
      else if( k == 2) {
        string[k] = '.';
      }
      else{
        hold = n / div;
        if(!hold && k < 1 && !val){
          string[k] = ' ';
        } 
        else{
          string[k] = hold + ASCI;
          val = 1;
        }
        n -= (hold * div);
        div /= 10;
      }
    }
    string[k] = 0;
  }
}

/****************Fixed_sDecOut3s***************
 converts fixed point number to the display
 format signed 32-bit with resolution 0.001
 range -9.999 to +9.999
 Input: signed 32-bit integer part of fixed point number
 Output: none
 Output to display has exactly 6 characters
 Examples
  2345 to " 2.345"  
 -8100 to "-8.100"
  -102 to "-0.102" 
    31 to " 0.031" 
 */ 
void Fixed_sDecOut3(int32_t n){
  char string[7];
  Fixed_sDecOut3s(n, string);
  printf("%s", string);
}

/**************Fixed_uBinOut8s***************
 unsigned 32-bit binary fixed-point with a resolution of 1/256. 
 The full-scale range is from 0 to 999.99. 
 If the integer part is larger than 256000, it signifies an error. 
 The Fixed_uBinOut8 function takes an unsigned 32-bit integer part 
 of the binary fixed-point number and outputs the fixed-point value on the OLED. 
 Input: unsigned 32-bit integer part of fixed point number
 Output: null-terminated string
Parameter output string
     0     "  0.00"
     2     "  0.01"
    64     "  0.25"
   100     "  0.39"
   50      "  1.95"
   512     "  2.00"
  5000     " 19.53"
 30000     "117.19"
255997     "999.99"
256000     "***.**"
*/
void Fixed_uBinOut8s(uint32_t n,  char *string) {
  n = ((n * 100) + 128) >> 8; 						// 128 is use to round to the nearest value because of 1/256 ie: [(n * 100) + 128] / 256  
  Fixed_uDecOut2s(n, string);
}	

/**************Fixed_uBinOut8***************
 unsigned 32-bit binary fixed-point with a resolution of 1/256. 
 The full-scale range is from 0 to 999.99. 
 If the integer part is larger than 256000, it signifies an error. 
 The Fixed_uBinOut8 function takes an unsigned 32-bit integer part 
 of the binary fixed-point number and outputs the fixed-point value on the OLED. 
 Input: unsigned 32-bit integer part of fixed point number
 Output: none
Parameter LCD display
     0	  0.00
     2	  0.01
    64	  0.25
   100	  0.39
   500	  1.95
   512	  2.00
  5000	 19.53
 30000	117.19
255997	999.99
256000	***.**
*/
void Fixed_uBinOut8(uint32_t n){
  char string[7];
  Fixed_uBinOut8s(n, string);
  printf("%s", string);
}
