//// Main.c 
//#include <stdint.h>
//#include "PLL.h"
//#include "Timer3.h"
//#include "can0.h"
//#include "Ping.h"
//#include "../inc/tm4c123gh6pm.h"
//#include "OS.h"
//#include "IR.h"
//#include "Interpreter.h"
//#include "UART.h"
//#include "inc/hw_types.h"
//#include <string.h>
//#include <stdio.h>

//#define PF0       (*((volatile uint32_t *)0x40025004))
//#define PF1       (*((volatile uint32_t *)0x40025008))
//#define PF2       (*((volatile uint32_t *)0x40025010))
//#define PF3       (*((volatile uint32_t *)0x40025020))
//#define PF4       (*((volatile uint32_t *)0x40025040))

//unsigned long NumCreated;   // number of foreground threads created

//#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

//void WaitForInterrupt(void);
//void DisableInterrupts(void); // Disable interrupts
//void EnableInterrupts(void);  // Enable interrupts
//void WaitForInterrupt(void);  // low power mode

//#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

////******** Interpreter **************
//// your intepreter from Lab 4 
//// foreground thread, accepts input from serial port, outputs to serial port
//// inputs:  none
//// outputs: none
//void Interpreter(void) {
//  Interpreter_Init();
//  while(1){
//	  Interpreter_Get();
//	}
//}

//char pingNum;

////*** PING Testing
//void Ping1TestThread(void) {
//  Ping1_Init();
//  OS_AddThread(&Ping1_PollThread, 127, 2);
//  while(1) {
//    long value = Ping_Fifo1_Get();
//    if (pingNum == 0) {
//      UART_printf("%d", value); UART_NewLine();
//    }
//  }
//}

//void Ping2TestThread(void) {
//  Ping2_Init();
//  OS_AddThread(&Ping2_PollThread, 127, 2);
//  while(1) {
//    long value = Ping_Fifo2_Get();
//    if (pingNum == 1) {
//      UART_printf("%d", value); UART_NewLine();
//    }
//  }
//}

//void Ping3TestThread(void) {
//  Ping3_Init();
//  OS_AddThread(&Ping3_PollThread, 127, 2);
//  while(1) {
//    long value = Ping_Fifo3_Get();
//    if (pingNum == 2) {
//      UART_printf("%d", value); UART_NewLine();
//    }
//  }
//}

//void Ping4TestThread(void) {
//  Ping4_Init();
//  OS_AddThread(&Ping4_PollThread, 127, 2);
//  while(1) {
//    long value = Ping_Fifo4_Get();
//    if (pingNum == 3) {
//      UART_printf("%d", value); UART_NewLine();
//    }
//  }
//}

//void SWPingTask(void) {
//  pingNum = (pingNum + 1) % 4;
//  UART_printf("Running ping %d", pingNum + 1); UART_NewLine();
//}

////*** CAN testing

//char run;
//void sendThread(void) {
//  uint8_t TxData[4];
//  run = 1;
//  int start = 0;
//  while(1) {
//    if (run) {
//      TxData[0] = start;
//      TxData[1] = start + 1;
//      TxData[2] = start + 2;
//      TxData[3] = start + 3;
//      start += 4;
//      CAN0_SendData(TxData);
//      OS_Sleep(1000);
//    }
//  }
//}

//void CANTestThread(void) {
//  CAN0_Open();
//  uint8_t RcvData[4];
//  uint32_t count = 0;
//  while(1) {
//    if (CAN0_GetMailNonBlock(RcvData)) {
//      count++;
//      for (int i = 0; i < 4; ++i) {
//        UART_OutUDec(RcvData[i]); UART_OutChar(' ');
//      }
//      UART_NewLine();
//      UART_printf("Received %d blocks", count);
//      UART_NewLine();
//    }
//  }
//}

//void SW1CANTask(void) {
//  if (run == 0) {
//    run = 1;
//  } else {
//    run = 0;
//  }
//}


//int testmain(void) {
//  OS_Init();           // initialize, disable interrupts
////  OS_AddThread(&Interpreter,128,3);
////	IR_Init();
////	OS_AddThread(&IRSensor0, 128, 2);
////  OS_AddThread(&CANThread, 128, 3);
////  OS_AddThread(&sendThread, 128, 3);
////  OS_AddSW1Task(&SW1CANTask, 1);
//  OS_AddThread(&Ping1TestThread, 128, 1);
//  OS_AddThread(&Ping2TestThread, 128, 1);
//  OS_AddThread(&Ping3TestThread, 128, 1);
//  OS_AddThread(&Ping4TestThread, 128, 1);
//  pingNum = 1;
//  OS_AddSWTask(&SWPingTask, 1);
//  OS_Launch(TIMESLICE);
//  return 1;
//}
