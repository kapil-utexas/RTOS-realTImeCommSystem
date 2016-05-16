//***********************  main.c  ***********************
// Program written by:
// - Steven Prickett  steven.prickett@gmail.com
//
// Brief desicription of program:
// - Initializes an ESP8266 module to act as a WiFi client
//   and fetch weather data from openweathermap.org
//
//*********************************************************
/* Modified by Jonathan Valvano
 Feb 3, 2016
 Out of the box: to make this work you must
 Step 1) Set parameters of your AP in lines 59-60 of esp8266.c
 Step 2) Change line 39 with directions in lines 40-42
 Step 3) Run a terminal emulator like Putty or TExasDisplay at
         115200 bits/sec, 8 bit, 1 stop, no flow control
 Step 4) Edit line 50 to match baud rate of your ESP8266 (9600 or 115200)
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/tm4c123gh6pm.h"

#include "pll.h"
#include "UART.h"
#include "esp8266.h"
#include "LED.h"

// prototypes for functions defined in startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

char Fetch[] = "GET /data/2.5/weather?q=Austin%20Texas&APPID=1234567890abcdef1234567890abcdef HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";
// 1) go to http://openweathermap.org/appid#use 
// 2) Register on the Sign up page
// 3) get an API key (APPID) replace the 1234567890abcdef1234567890abcdef with your APPID
uint8_t awesome = 1;
uint32_t sendPacketCount =0;
char buffer[1024];
int main(void){  
  DisableInterrupts();
  PLL_Init(Bus80MHz);
  LED_Init();  
  Output_Init();       // UART0 only used for debugging
  printf("\n\r-----------\n\rSystem starting...\n\r");
  ESP8266_Init(115200);  // connect to access point, set up as client
  ESP8266_GetVersionNumber();
	//server is 2
	uint32_t packetsDropped = 0;
	uint8_t connected= 0; 
	while(1){
   // ESP8266_GetStatus();
    LED_RedOff();
					LED_GreenOff();
 
			 
		if(ESP8266_MakeTCPConnection("172.20.10.10")){ // open socket in server
      
			
      //ESP8266_SendTCP(Fetch);
			//send a http client response
			connected = 1;
			sendPacketCount++;
			sprintf((char*)buffer, "I am the %d packet",sendPacketCount );
		
		ESP8266_SendClientResponse(buffer);
			
		//DelayMs(200);
			LED_RedOn();
			ESP8266SendCommand("AT+CIPCLOSE\r\n");  
		
			//DelayMs(500);
			
 LED_RedOff();
			LED_GreenOn(); 
			
    }
		    while(Board_Input()==0){// wait for touch
				}; 
//		else{
//		packetsDropped ++;
//		printf("%d packets are dropped",packetsDropped);}
//    LED_GreenOn();
//  //  LED_RedOff();
//		
//		
//	LED_GreenOff();
//	}
}}


//	else{
//			if(ESP8266_MakeTCPConnection("172.20.10.2")){ // open socket in server
//					LED_RedOn();
//			LED_GreenOff();

//		//ESP8266_SendTCP(Fetch);
//		//send a http client response
//		ESP8266_SendClientResponse("I am the FIRST!!!");

//		}
//					while(Board_Input()==0){// wait for touch
//	}; 
//	LED_GreenOn();
//	LED_RedOff();
//	}
//	//ESP8266_CloseTCPConnection();

//}
//}

//// transparent mode for testing
//void ESP8266SendCommand(char *);
//int main2(void){  char data;
//  DisableInterrupts();
//  PLL_Init(Bus80MHz);
//  LED_Init();  
//  Output_Init();       // UART0 as a terminal
//  printf("\n\r-----------\n\rSystem starting at 115200 baud...\n\r");
////  ESP8266_Init(38400);
//  ESP8266_InitUART(9600,true);
//  ESP8266_EnableRXInterrupt();
//  EnableInterrupts();
//  ESP8266SendCommand("AT+RST\r\n");
//  data = UART_InChar();
////  ESP8266SendCommand("AT+UART=115200,8,1,0,3\r\n");
////  data = UART_InChar();
////  ESP8266_InitUART(115200,true);
////  data = UART_InChar();
//  
//  while(1){
//// echo data back and forth
//    data = UART_InCharNonBlock();
//    if(data){
//      ESP8266_PrintChar(data);
//    }
//  }
//}




