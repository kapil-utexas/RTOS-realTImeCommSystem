// OS_Shared.h
// Allen Wang and Alvin Tung
// Used to facilitate shared variables

#ifndef _OS_SHARED_H_
#define _OS_SHARED_H_ 1

extern unsigned long SystemTimeMS; 

#ifdef DEBUG
// Code for calculating disable/enable time
extern unsigned long long disableTime;
extern unsigned long long enableTime;
extern unsigned long maxDisableTime;
extern unsigned long lastDisableTime;
extern unsigned long lastEnableTime;
extern uint32_t interruptsEnabled;
extern uint32_t continueMeasuring;
extern uint32_t OS_IsRunning;
extern Sema4Type ST7735Mutex;
#endif 

#endif /* OS_SHARED_H_ */
