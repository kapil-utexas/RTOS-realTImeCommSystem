// Heartbeat.h
// Allen Wang and Alvin Tung
// Library used for LED toggles
#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

//************* Heartbeat_Init *******************
// Heartbeat initializations. Uses PF2.
// Input - none
// Output - none
void HeartBeat_Init(void);

//************* Heartbeat_Toggle *******************
// Toggles PF2
// Input - none
// Output - none
void HeartBeat_Toggle(void);

//************* Hardbeat_Init *******************
// Hardbeat initializations. Uses PF1.
// Input - none
// Output - none
void HardBeat_Init(void);

//************* Hardbeat_Toggle *******************
// Toggles PF1
// Input - none
// Output - none
void HardBeat_Toggle(void);

#endif /* _HEARTBEAT_H_ */
