// OS_Time.h
// Allen Wang and Alvin Tung
// This file is used for all OS timing related functions

#ifndef _OS_TIME_H_
#define _OS_TIME_H_ 1

#include <stdint.h>

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void);

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long stop, unsigned long start);

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void);

// ******** OS_TimeToMSTime ************
// Converts this time to an MS time.
// Inputs:  Desired time
// Outputs: Time in ms
uint32_t OS_TimeToMSTime(unsigned long time);

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void);

// ******* OS_UpdateTimeInfoOnEnable **********
// Used in OS_EnableInterrupts and OS_EndCritical
// Used to keep track of how much time is spent when interrupts are enabled/disabled.
void OS_UpdateTimeInfoOnEnable(void);

// ******* OS_UpdateTimeInfoOnDisable **********
// Used in OS_DisableInterrupts and OS_StartCritical
// Used to keep track of how much time is spent when interrupts are enabled.
void OS_UpdateTimeInfoOnDisable(void);

// ******* OS_UpdateInterruptStatus **********
// Used in OS_EndCritical. Pass in the status and determine whether or not interrupts are enabled.
void UpdateInterruptStatus(unsigned long status);
#endif /*_OS_TIME_H_ */
