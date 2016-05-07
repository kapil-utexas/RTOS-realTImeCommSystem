// OS_Time.c
// Allen Wang and Alvin Tung
// This file is used for all OS timing related functions
#include <stdint.h>
#include "OS_Time.h"
#include "OS_Shared.h"
#include "inc/tm4c123gh6pm.h"

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void) {
	return SystemTimeMS*80000 + ((80000 - 1) - TIMER2_TAV_R);
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long stop, unsigned long start) {
	return (stop - start);
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void) {
   SystemTimeMS = 0;
}

// ******** OS_TimeToMSTime ************
// Converts this time to an MS time.
// Inputs:  Desired time
// Outputs: Time in ms
uint32_t OS_TimeToMSTime(unsigned long time) {
  return time / 80000;
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void) {
     return SystemTimeMS;
}

extern unsigned long long disableTime;
extern unsigned long long enableTime;
extern unsigned long maxDisableTime;
extern unsigned long lastDisableTime;
extern unsigned long lastEnableTime;
extern uint32_t interruptsEnabled;
extern uint32_t continueMeasuring;
extern uint32_t OS_IsRunning;

#define UINT32_T_MAX    0xFFFFFFFF

// ******* OS_UpdateTimeInfoOnEnable **********
// Used in OS_EnableInterrupts and OS_EndCritical
// Used to keep track of how much time is spent when interrupts are disabled.
void OS_UpdateTimeInfoOnEnable(void) {
  if (continueMeasuring && OS_IsRunning) {
    unsigned long thisTime = OS_Time();
    // this is called when interrupts are going to be enabled
    // keep track of disabled time
    if (!interruptsEnabled) { // interrupts were disabled, calculate time spent in disabled mode
      if (lastDisableTime != 0) { 
        if (thisTime >= lastDisableTime) {
          unsigned long diff = OS_TimeDifference(thisTime, lastDisableTime);
          if (disableTime > UINT32_T_MAX*80000 - diff) { 
            continueMeasuring = 0; 
          }
          disableTime += diff;
          if (diff > maxDisableTime) { 
            maxDisableTime = diff; 
          }
        } // else there was an overflow in the timer
      } // else this is the first time enabling interrupts. Don't calculate anything
      //lastEnableTime = thisTime;
    } // else do nothing
    lastEnableTime = thisTime;
  }
  interruptsEnabled = 1;
}

// ******* OS_UpdateTimeInfoOnDisable **********
// Used in OS_DisableInterrupts and OS_StartCritical
// Used to keep track of how much time is spent when interrupts are enabled.
void OS_UpdateTimeInfoOnDisable(void) {
  if (continueMeasuring && OS_IsRunning) {
    unsigned long thisTime = OS_Time();
    if (interruptsEnabled) {  // interrupts were enabled. Calculate time spent in disabled mode
      if (lastEnableTime != 0) {
        if (thisTime >= lastEnableTime) {
          unsigned long diff = OS_TimeDifference(thisTime, lastEnableTime);
          if (enableTime > UINT32_T_MAX*80000 - diff) { 
            continueMeasuring = 0; 
          }    // overflowed
          enableTime += diff;
        } // else there was an overflow in the timer
      } // else this was the first time disabling interrupts. Don't calculate anything      
      //lastDisableTime = thisTime;
    }
    lastDisableTime = thisTime;
  }
  interruptsEnabled = 0;
}

// ******* OS_UpdateInterruptStatus **********
// Used in OS_EndCritical. Pass in the status and determine whether or not interrupts are enabled.
void UpdateInterruptStatus(unsigned long status) {
  if (status) interruptsEnabled = 0;
  else interruptsEnabled = 1;
}
