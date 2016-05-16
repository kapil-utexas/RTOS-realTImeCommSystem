// SensorInstructions.h
#ifndef _SENSOR_INSTRUCTIONS_H_
#define _SENSOR_INSTRUCTIONS_H_

#include <stdint.h>
typedef struct SensorInstructionFormat {
  uint8_t angle;
  uint8_t speed;
  uint8_t direction;
} SensorInstruction;


#endif /* _SENSOR_INSTRUCTIONS_H_ */
