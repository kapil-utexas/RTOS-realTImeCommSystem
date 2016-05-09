// Main.c 
#include <stdint.h>
#include "PLL.h"
#include "Timer3.h"
#include "can0.h"
#include "Ping.h"
#include "../inc/tm4c123gh6pm.h"
#include "OS.h"
#include "IR.h"
#include "Interpreter.h"
#include "UART.h"
#include "inc/hw_types.h"
#include "SensorInstructions.h"
#include "DCMotor.h"
#include "ST7735.h"
#include <string.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pll.h"
#include "esp8266.h"
#include "LED.h"


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))

/*
* 2 Ping sensors facing the front     (J10Y and J11Y)
* IR0 points forward left diagonally  (PE3) J5
* IR1 points directly left straight   (PE2) J6
* IR2 points forward right diagonally (PE1) J7
* IR3 points directly right straight  (PE0) J8
*/


void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
short PID_stm32(short Error, short *Coeff);


unsigned long NumCreated;   // number of foreground threads created

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

void WaitForInterrupt(void);
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

//******** Interpreter **************
// your intepreter from Lab 4 
// foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none
void Interpreter(void) {
  Interpreter_Init();
  while(1){
	  Interpreter_Get();
	}
}

#define PING_THREAD_PRIORITY 1
#define IR_THREAD_PRIORITY   1

void AddPingThreads(void) {     // comment out the pings you don't want
//  OS_AddThread(&Ping1_PollThread, 127, PING_THREAD_PRIORITY);
//  OS_AddThread(&Ping2_PollThread, 127, PING_THREAD_PRIORITY);
  OS_AddThread(&Ping3_PollThread, 127, PING_THREAD_PRIORITY);
//  OS_AddThread(&Ping4_PollThread, 127, PING_THREAD_PRIORITY);
}

void AddIRThreads(void) {   
  OS_AddThread(&IRSensor0Thread, 127, IR_THREAD_PRIORITY);
  OS_AddThread(&IRSensor1Thread, 127, IR_THREAD_PRIORITY);
  OS_AddThread(&IRSensor2Thread, 127, IR_THREAD_PRIORITY);
  OS_AddThread(&IRSensor3Thread, 127, IR_THREAD_PRIORITY);
}

void CANSensorSendThread(void) {
  CAN0_Open();
  uint8_t TxData[4];
  while(1) {
    SensorInstruction instr = OS_MailBox_Recv();
    TxData[0] = instr.angle;
    TxData[1] = instr.speed;
    TxData[2] = instr.direction;
    TxData[3] = 0;
    CAN0_SendData(TxData);
    
    #if DEBUG == 1
      ST7735_SClear(0, 0);
      ST7735_SClear(0, 1);
      ST7735_SClear(0, 2);
      ST7735_SDrawString(0, 0, "Angle: "); ST7735_SDrawValue(0, 0, instr.angle);
      ST7735_SDrawString(0, 1, "Speed: "); ST7735_SDrawValue(0, 1, instr.speed);
      ST7735_SDrawString(0, 2, "Direction: "); ST7735_SDrawValue(0, 2, instr.direction);
//      UART_printf("Angle: "); UART_OutUDec(instr.angle); UART_NewLine();
//      UART_printf("Speed: "); UART_OutUDec(instr.speed); UART_NewLine();
//      UART_printf("Direction: "); UART_OutUDec(instr.direction); UART_NewLine();
    #endif
  }
}


#define MAX_SPEED_DISTANCE_THRESHOLD   300     // can only go max speed if greater than this value
#define MED_SPEED_DISTANCE_THRESHOLD   120
#define FLOOR_SPEED_DISTANCE_THRESHOLD 60     // 0 - this value is stop

// this depends on the configuration
#define X_L                           75     // Calibrate this
#define X_R                           75 


// TURNING CONSTANTS (angle is a percentage out of TOTALPERCENTAGE (200))
#define K_TURN_NUM                    1     // change this to decide sensitivity of turn! (origianally K = .5 or (10/20), but since 200% made it to K = 1)
#define K_TURN_DEN                    10     // change this to decide sensitivity of turn!
#define TOO_CLOSE                     18    // change this to decide distance from wall!
#define TOO_FAR                       60    // change this to decide how close to hug the desired wall

// helper function
int min(int a, int b, int c, int d) {
  if (a < b && a < c && a < d) {
    return a;
  } else if (b < a && b < c && b < d) {
    return b;
  } else if (c < a && c < b && c < d) {
    return c;
  } else {
    return d;
  }
}

static unsigned long runningTime;       // (in terms of 100 ms)

short Coeff[3]; // PID Coefficients
short Actuator;
void ProcessSensors(void)  {
//  Coeff[0] = 384;   // 1.5 = 384/256 proportional coefficient
//  Coeff[1] = 128;   // 0.5 = 128/256 integral coefficient
//  Coeff[2] = 64;    // 0.25 = 64/256 derivative coefficient
  runningTime = 0;
  
  int8_t angle = STRAIGHT; // initialize angle to straight value to begin 
  while (1) {
//SENSOR VALUES    
//    unsigned long Ping1Value = Ping_Fifo1_Get();
//    unsigned long Ping2Value = Ping_Fifo2_Get();
    unsigned long Ping3Value = Ping_Fifo3_Get();
//    unsigned long Ping4Value = Ping_Fifo4_Get();
    unsigned long IR0Value = IR_Fifo0_Get();
    unsigned long IR1Value = IR_Fifo1_Get();
    unsigned long IR2Value = IR_Fifo2_Get();
    unsigned long IR3Value = IR_Fifo3_Get();
    uint8_t t = 0;
    
//    unsigned long avgForwardDistance = (Ping3Value + Ping2Value) / 2;
    unsigned long avgForwardDistance = (Ping3Value);

//CALCULATIONS
    
    uint8_t speed;
    uint8_t direction;
//    uint8_t angle; // moving angle to global to allow for P controller (saves past value)
    SensorInstruction instr;

    // ***** Calculate Speed    *****
    // Feel free to change this implementation later. This is just a prototype
    if (avgForwardDistance > MAX_SPEED_DISTANCE_THRESHOLD) {
      // Go max speed
      speed = DCM_SLOW_SPEED;
    } else if (avgForwardDistance < FLOOR_SPEED_DISTANCE_THRESHOLD) {
      speed = 0;
    } else {
      //speed = (DCM_MAX_SPEED * (avgForwardDistance)) / (MAX_SPEED_DISTANCE_THRESHOLD);
      if (avgForwardDistance > MAX_SPEED_DISTANCE_THRESHOLD) {
        speed = DCM_MAX_SPEED;
      } else if (avgForwardDistance > MED_SPEED_DISTANCE_THRESHOLD) {
        speed = DCM_MED_SPEED;
      } else {
        speed = DCM_SLOW_SPEED;
      }
      
    }
        
    // ***** Calculate Direction *****
    // just assume forward for now..
    direction = DCM_FORWARD;

    // ***** Calculate Angle     *****
    long a_L = IR0Value;
    long b_L = IR1Value;
    long a_R = IR2Value;
    long b_R = IR3Value;
    volatile long desired_angle = 0;
    volatile long measured_angle;
    volatile long error = 0;
    volatile short tracked;
    
    // Protect against divide by 0 errors
    if (a_L == 0) { 
      a_L = 1;
    }
    if (a_R == 0) {
      a_R = 1;
    }
    
    // ****** NOTE: To change sensitivity of turn change only:
    // ******       ERR_THRESH to make it more or less sensitive to error (larger less sensitive, smaller more sensitive)
    // ******       
    if (a_R + b_R < a_L + b_L) {  // track the right wall
      tracked = 0;
      measured_angle = (100 * b_R / a_R);
      error = measured_angle - X_R;

      
      long error_term = (1)*error*K_TURN_NUM / K_TURN_DEN;
      angle = error_term + angle;// STRAIGHT;

      if (angle > HARD_RIGHT) {
        angle = HARD_RIGHT;
      } else if (angle < HARD_LEFT) { 
        angle = HARD_LEFT;
      } if(error < 10 && error > -10) {
        angle = STRAIGHT;
      }        
    } else {                      // track the left wall
      tracked = 1;
      measured_angle = (100 * b_L) / a_L;
      error = measured_angle - X_L;
      
      long error_term = (-1)*error*K_TURN_NUM / K_TURN_DEN;
      
//      angle = (long)(((float)(1)*error*(K_TURN_NUM))/ K_TURN_DEN) + angle; // use past angle to calculate changes (-1 becuase it needs to do opposite as right wall tracking)
      angle = error_term + angle; // use past angle to calculate changes (-1 becuase it needs to do opposite as right wall tracking)

      
      if(angle < HARD_LEFT) {
        angle = HARD_LEFT;
      } else if(angle > HARD_RIGHT){ 
        angle = HARD_RIGHT;
      } if(error < 10 && error > -10) {
        angle = STRAIGHT;
      }  
    }
    
    if (a_R < TOO_CLOSE || b_R < TOO_CLOSE || a_L < TOO_CLOSE || b_L < TOO_CLOSE) {
      t = 1;
      int m = min(a_R, b_R, a_L, b_L);
      if (m == a_R || m == b_R) {
        angle = HARD_RIGHT;
      } else {
        angle = HARD_LEFT;
      }
    } 
/*    else if(tracked == 0 && (a_R > TOO_FAR || b_R > TOO_FAR)){ // tracking right
      t = 1; // used only for debugging
      angle = HARD_RIGHT;
    }
    else if(tracked == 1 && (a_L > TOO_FAR || b_L > TOO_FAR)){
      t = 1; 
      angle = HARD_LEFT;
    } */ // CONSIDER USING THIS CODE IF YOU WANT TO TRACK SAME WALL TO MANUEVER AWAY LATER
    
    // ***** Send to Mailbox     *****
    
    instr.speed = speed;
    instr.direction = direction;
    instr.angle = angle;
    
    
    // Running time for checkout only!
//    if (runningTime >= 1800) {
//      instr.speed = 0;
//      instr.direction = DCM_FORWARD;
//      instr.angle = STRAIGHT;
//    }
    
    OS_MailBox_Send(instr);
    
    #if DEBUG == 1
      ST7735_SClear(1, 0);
      ST7735_SDrawString(1, 0, "Diagonal left: "); ST7735_SDrawValue(1, 0, a_L);

      ST7735_SClear(1, 1);
      ST7735_SDrawString(1, 1, "Left: "); ST7735_SDrawValue(1, 1, b_L);

      ST7735_SClear(1, 2);
      ST7735_SDrawString(1, 2, "Diagonal right: "); ST7735_SDrawValue(1, 2, a_R);

      ST7735_SClear(1, 3);
      ST7735_SDrawString(1, 3, "Right: "); ST7735_SDrawValue(1, 3, b_R);

      ST7735_SClear(1, 4);
      ST7735_SDrawString(1, 4, "Ping: "); ST7735_SDrawValue(1, 4, Ping3Value);
      
      ST7735_SClear(1, 5);
      if (angle == SOFT_LEFT || angle == HARD_LEFT) {
        ST7735_SDrawString(1, 5, "Left");
      } else if (angle == SOFT_RIGHT || angle == HARD_RIGHT) {
        ST7735_SDrawString(1, 5, "Right");
      } else {
        ST7735_SDrawString(1, 5, "Straight");
      }
      if (t == 1) {
        ST7735_SDrawString(1,5,"*");
      }
      
      ST7735_SClear(1,6);
      ST7735_SDrawString(1,6,"Error: "); ST7735_SDrawValue(1,6,error);
      ST7735_SClear(1, 7);
//      ST7735_SDrawString(1,7,"Current Speed: "); ST7735_SDrawValue(1,7,speed);
//      ST7735_SDrawString(1,7,"Counter: "); ST7735_SDrawValue(1,7,runningTime);
      ST7735_SDrawString(1,7,"Angle: "); ST7735_SDrawValue(1,7,instr.angle);

    #endif
    runningTime++;
    OS_Sleep(POLL_RATE);
  }
}

//short IntTerm;     // accumulated error, RPM-sec
//short PrevError;   // previous error, RPM
//short Coeff[3];    // PID coefficients
//short Actuator;
//unsigned long PIDWork;
//void PID(void){ 
//short err;  // speed error, range -100 to 100 RPM
//  PIDWork = 0;
//  IntTerm = 0;
//  PrevError = 0;
//  Coeff[0] = 384;   // 1.5 = 384/256 proportional coefficient
//  Coeff[1] = 128;   // 0.5 = 128/256 integral coefficient
//  Coeff[2] = 64;    // 0.25 = 64/256 derivative coefficient*
//  while(1) { 
//    for(err = -1000; err <= 1000; err++){    // made-up data
//      Actuator = PID_stm32(err,Coeff)/256;
//    }

//    PIDWork++;        // calculation finished
//  }
////  for(;;){ }          // done
//}

// ********************* MOTOR THREADS ********************

// data sent is:
// Angle - servo
// Speed
// Direction - forward/back
// Nothing

void CANMotorThread(void) {
  CAN0_Open();
  uint8_t RcvData[4];
  while(1) {
    if (CAN0_GetMailNonBlock(RcvData)) {
      uint8_t angle = RcvData[0];
      uint8_t speed = RcvData[1];
      uint8_t direction = RcvData[2];
//      UART_printf("Angle: "); UART_OutUDec(angle); UART_NewLine();
//      UART_printf("Speed: "); UART_OutUDec(speed); UART_NewLine();
//      UART_printf("Direction: "); UART_OutUDec(direction); UART_NewLine();
      SensorInstruction instr;
      instr.angle = angle;
      instr.speed = speed;
      instr.direction = direction;
      OS_MailBox_Send(instr);
      // do something with this
    }
  }
}

void MotorProcessor(void) {
  Motor_Init();
  while(1) {
    SensorInstruction instr = OS_MailBox_Recv();
    char pow = instr.speed;
//    if (pow > DCM_MAX_SPEED) {
//      pow = DCM_MAX_SPEED;
//    }
    
    // char dir = instr.direction;

    Motor_Run(rightM, pow, forward);
    char angle = instr.angle;
    Motor_Direction(angle);
  }
}

//#if MOTOR_BOARD == 0
//int main(void) {     // sensor board main
//  OS_Init();
//  IR_Init();
//  Ping_Init();

//  // TODO - Change the priorities later
//  #if DEBUG == 1
//    ST7735_SInit();
//  #endif
//  OS_MailBox_Init();
//  AddIRThreads();
//  AddPingThreads();
//  OS_AddThread(&CANSensorSendThread, 128, 1);
//  OS_AddThread(&ProcessSensors, 128, 1);  
////  OS_AddThread(&Interpreter, 128, 1);
//  OS_Launch(TIMESLICE);
//}
//#else
//int main(void) {      // motor board main
//  OS_Init();
//  OS_MailBox_Init();
//  OS_AddThread(&CANMotorThread, 128, 2);
//  OS_AddThread(&MotorProcessor, 128, 1);
//  OS_Launch(TIMESLICE);
//}
//#endif

// prototypes for functions defined in startup.s
//void DisableInterrupts(void); // Disable interrupts
//void EnableInterrupts(void);  // Enable interrupts
//long StartCritical (void);    // previous I bit, disable interrupts
//void EndCritical(long sr);    // restore I bit to previous value
//void WaitForInterrupt(void);  // low power mode
char Fetch[] = "GET /data/2.5/weather?q=Austin%20Texas&APPID=e0493977c4479cee13a011dc229cf26c HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";
//char Fetch[] = "GET HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";

void LED_BlueOff(void){
  PF2 = 0x00;
}
void doTCP(){
	while(1){
		LED_BlueOff();
    //ESP8266_GetStatus();
	  LED_GreenOff();
		if( ESP8266_MakeTCPConnection("172.20.10.4") ){ // open socket in server
			LED_RedOn();
			
			 //UART_printf("\n\r-----------\n\rSystem starting...\n\r");
			//DelayMs(500);
      //ESP8266_SendTCP(Fetch);
//			//HTTP_ServePage("AWESOME!!!!");
			ESP8266_SendClientResponse("AWESOME!!!!");
			
    }
    //ESP8266_CloseTCPConnection();
		LED_RedOff();
		LED_GreenOn();
		
//		  ST7735_SClear(1,0);
//      ST7735_SDrawString(1,0,"Error: "); 
//      ST7735_SClear(1, 7);

    while(Board_Input()==0){// wait for touch
    }; 
    
		LED_BlueOff();
    //LED_RedToggle();
	}
	}
// 1) go to http://openweathermap.org/appid#use 
// 2) Register on the Sign up page
// 3) get an API key (APPID) replace the 1234567890abcdef1234567890abcdef with your APPID

int main(void){
  OS_Init();
//  IR_Init();
//  Ping_Init();

//  // TODO - Change the priorities later
//  #if DEBUG == 1
   // ST7735_SInit();
  LED_Init();

	//  #endif
//  OS_MailBox_Init();
  OS_AddThread(&doTCP, 128, 1);
	//  OS_AddThread(&ProcessSensors, 128, 1);  
////  OS_AddThread(&Interpreter, 128, 1);
  OS_Launch(TIMESLICE);
  
  //DisableInterrupts();
  //PLL_Init(Bus80MHz);
	//OS_AddThread(&CANSensorSendThread, 128, 1);	
  
	//Output_Init();       // UART0 only used for debugging
  //printf("\n\r-----------\n\rSystem starting...\n\r");
 // ESP8266_Init(115200);  // connect to access point, set up as client
  //ESP8266_GetVersionNumber();
	

}
