// Interpreter.c
// Allen Wang and Alvin Tung
// This is our interpreter which is helpful for debugging purposes

#include "ff.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "UART.h"
#include "ST7735.h"
#include "Interpreter.h"
#include "ADC.h"
#include "fixed.h"
#include "OS_Shared.h"
#include "OS_Time.h"
#include "OS.h"

#define BUF_SIZE        50
#define ARG_MAX_SIZE    30
#define MAX_NUM_PARAMS  5
static char curr_buf_size= 0;

static FATFS g_sFatFs;
FRESULT Fresult;

void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts

//extern Sema4Type ScreenMutex; // to protect sd writes

extern unsigned long long disableTime;
extern unsigned long long enableTime;
extern unsigned long maxDisableTime;
extern unsigned long lastDisableTime;
extern unsigned long lastEnableTime;
extern uint32_t interruptsEnabled;
extern uint32_t continueMeasuring;

// Used for function tags
enum fun { 
	SI, SC, SS, SV, 
	AI, AO, AN, AS, AC, 
  TI, TR, JI, 
  FF, FM, FR, FW, FD, FC, FN,
  LS, MKD, CD, PWD, 
  DP, 
  ERROR };
typedef enum fun functionType;

typedef struct functionInfo {
  functionType f;
  int argc;
  char argv[MAX_NUM_PARAMS][ARG_MAX_SIZE];
} FunctionInfo;

void nl(void) {
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void println(char * s) {
	UART_printf("%s", s); nl();
}
//---------------Interpreter_Init------------------
// Initializations for the interpreter
// Input: none
// Ouput: none
void Interpreter_Init(void) {
  UART_printf("Booting..."); nl();
  UART_printf("Welcome to AWT, the RTOS created for EE445M"); nl();
  UART_printf("Created by Allen Wang and Alvin Tung"); nl();
}

#define isWhitespace(c) (c == ' ') //|| c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

static void skipWhitespace(char * s, int * idx) {
	while (s[*idx] && isWhitespace(s[*idx]) && *idx < BUF_SIZE) {
		(*idx)++;
	}
}

static int getNextToken(char * s, char * buf, int * idx) {
  skipWhitespace(s, idx);
  if (!s[*idx]) { return 0; }
  int bufferCounter = 0;
    while (s[*idx] && !isWhitespace(s[*idx]) && bufferCounter < ARG_MAX_SIZE - 1) {
        buf[bufferCounter] = s[*idx];
        (*idx)++;
        bufferCounter++;
    }
    buf[bufferCounter] = 0;
		return 1;
}

static void lower(char * s) {
	while (*s) {
		if (*s >= 'A' && *s <= 'Z') {
			*s = *s - 'A' + 'a';
		}
    s++;
	}
}

functionType getFunctionType(char * s) {
  lower(s);
  if (strcmp(s, "si") == 0) { return SI; }
  if (strcmp(s, "sc") == 0) { return SC; }
  if (strcmp(s, "ss") == 0) { return SS; }
  if (strcmp(s, "sv") == 0) { return SV; }
  if (strcmp(s, "ai") == 0) { return AI; }
  if (strcmp(s, "ao") == 0) { return AO; }
  if (strcmp(s, "an") == 0) { return AN; }
  if (strcmp(s, "as") == 0) { return AS; }
  if (strcmp(s, "ac") == 0) { return AC; }
  if (strcmp(s, "ti") == 0) { return TI; }
  if (strcmp(s, "ji") == 0) { return JI; }
  if (strcmp(s, "tr") == 0) { return TR; }
  if (strcmp(s, "format") == 0) { return FF; }
  if (strcmp(s, "mount") == 0){ return FM;}
  if (strcmp(s, "less") == 0) { return FR; }
  if (strcmp(s, "write") == 0) { return FW; }
  if (strcmp(s, "rm") == 0) { return FD; }
  if (strcmp(s, "truncate") == 0) { return FC; }
  if (strcmp(s, "touch") == 0) { return FN; }
  if (strcmp(s, "ls") == 0) { return LS; }
  
  if (strcmp(s, "mkdir") == 0) { return MKD; }
  if (strcmp(s, "cd") == 0) { return CD; }
  if (strcmp(s, "pwd") == 0) { return PWD; }
  if (strcmp(s, "poll") == 0) { return DP; }    // distance poll
  UART_printf("AWT: %s: command not found", s); nl();
  return ERROR;
}

static FunctionInfo parse(char * s, int * err) {
  int idx = 0;
  int argc = 0;
  FunctionInfo f;
  char tokens[MAX_NUM_PARAMS + 1 ][ARG_MAX_SIZE] = { 0 };
  while (s[idx]) {
    int success = getNextToken(s, tokens[argc], &idx);
		if (success) { ++argc; }
  }
  --argc;     // Ignore the first token (function type)
		
  functionType ft = getFunctionType(tokens[0]);

  f.f = ft;
  f.argc = argc;

  for (int i = 0; i < argc; ++i) {
    strcpy(f.argv[i], tokens[i + 1]);
  }

  if (ft == ERROR) { *err = 1; }
  else { *err = 0; }
  return f;
}

#define isNumber(c) (c >= '0' && c <= '9')
int stringToInt(char * s, int * err) {
  int intBuilder = 0;
  *err = 1;
  if (!*s) return 0;        // empty string
  while (*s) { 
    intBuilder *= 10;
    if (!isNumber(*s)) { return 0; }
    intBuilder += *s - '0';
    ++s;
  }
  *err = 0;
  return intBuilder;
}

void paramsError(int expectedParamsCount) {
  UART_printf("AWT: expected %d parameter(s)", expectedParamsCount); nl();
} 

#define FILE_BUFFER_MAX_LENGTH    200
char file_buffer[FILE_BUFFER_MAX_LENGTH];

#define PATH_SIZE 50
char path[PATH_SIZE];

char currentPath[PATH_SIZE];

unsigned short ADCBuffer[BUF_SIZE];
void runFunction(FunctionInfo f) {
  int args[MAX_NUM_PARAMS] = { 0 };
  int argsi = 0;
  char v[7];
  for (int i = 0; i < f.argc; ++i) { // parse all possible ints
    int err = 0;
    int v = stringToInt(f.argv[i], &err);
    if (err) { continue; }
      args[argsi] = v;
      ++argsi;
  }
  if (f.f == SI) {
    if (f.argc != 0) { paramsError(0); return; }
    ST7735_SInit();
    println("LCD Initialized");
  } else if (f.f == SC) {
  if (f.argc != 2) { paramsError(2); return; }
    ST7735_SClear(args[0], args[1]);
    UART_printf("Cleared device %d on line %d", args[0], args[1]); nl();
  } else if (f.f == SS) {
    if (f.argc < 3) { paramsError(3); return; }
    ST7735_SDrawString(args[0], args[1], f.argv[2]);
    UART_printf("Printed %s on device %d, line %d", f.argv[2], args[0], args[1]); nl();
  } else if (f.f == SV) {
    if (f.argc != 3) { paramsError(3); return; }
    ST7735_SDrawValue(args[0], args[1], args[2]);
    UART_printf("Printed %d on device %d, line %d", args[0], args[1], args[2]); nl();
  } else if (f.f == AI) {
    if (f.argc != 1) { paramsError(1); return; }
    ADC_Init(args[0]);
    UART_printf("Initialized ADC %d", args[0]); nl();
  } else if (f.f == AO) {
    if (f.argc != 1) { paramsError(1); return; }
    ADC_Open(args[0]);
    UART_printf("Changed ADC Channel to %d", args[0]); nl();
  } else if (f.f == AN) { 
    if (f.argc != 0) { paramsError(0); return; }
    int ai = ADC_In();
    int converted = ADC_ConvertToVolts(ai);
    Fixed_uDecOut2s(converted, v);
    UART_printf("ADC value is %s", v); nl();
  } else if (f.f == AS) { 
    if (f.argc != 0) { paramsError(0); return; }
    int status = ADC_Status();
    if (status) { println("ADC is collecting samples"); }
    else {
      println("ADC is done collecting samples: ");
      for (int i = 0; i < curr_buf_size - 1; ++i) {
				Fixed_uDecOut2s(ADC_ConvertToVolts(ADCBuffer[i]), v);	
        UART_OutString(v); 
				UART_OutString(", "); 
        //UART_printf("%s, ", v);
        //UART_printf("%d ", ADCBuffer[i]);
      }
      if (curr_buf_size > 0) {
        Fixed_uDecOut2s(ADC_ConvertToVolts(ADCBuffer[curr_buf_size - 1]), v);	
        UART_OutString(v); nl();
      }
    }
  } else if (f.f == AC) { 
    UART_printf("ADC Collect is not functional right now"); nl();
//    if (f.argc != 3) { paramsError(3); return; }
//    if (args[2] > BUF_SIZE) { println("Cannot support this many samples"); return; }
//    int status = ADC_Collect(args[0], args[1], ADCBuffer, args[2]);
//    curr_buf_size = args[2];
//    if (status) { println("ADC is collecting samples"); }
//    else {
//      println("ADC is done collecting samples");
//      for(int i = 0; i < curr_buf_size - 1; i++){
//        Fixed_uDecOut2s(ADC_ConvertToVolts(ADCBuffer[i]), v);	
//        UART_OutString(v); UART_OutString(", ");
//      }
//      if(curr_buf_size > 0){
//        Fixed_uDecOut2s(ADC_ConvertToVolts(ADCBuffer[curr_buf_size - 1]), v);		
//        UART_OutString(v); nl();
//      }
//    }    
  } else if (f.f == TI) {   // print system time info (time disabled vs time enabled)
    if (f.argc != 0) { paramsError(0); return; }
    uint32_t msTimeDisabled = OS_TimeToMSTime(disableTime);
    uint32_t msTimeEnabled = OS_TimeToMSTime(enableTime);
    uint32_t msMaxDisableTime = OS_TimeToMSTime(maxDisableTime);
    UART_printf("Time spent in disabled interrupt mode: "); UART_OutUDec(msTimeDisabled); UART_printf(" ms"); nl();
    UART_printf("Time spent in enabled interrupt mode: "); UART_OutUDec(msTimeEnabled); UART_printf(" ms"); nl();
    UART_printf("Max time spent in disabled interrupt mode: "); UART_OutUDec(msMaxDisableTime); UART_printf(" ms"); nl();
  } else if (f.f == TR) {
    if (f.argc != 0) { paramsError(0); return; }
    UART_printf("Resetting timing info..."); nl();
    disableTime = 0;
    enableTime = 0;
    maxDisableTime = 0;
    continueMeasuring = 1;
    lastDisableTime = 0;
    lastEnableTime = 0;
  } else if (f.f == JI) {   // Prints jitter histogram
    if (f.argc != 0) { paramsError(0); return; }
    OS_UARTPrintJitterInfo(); nl();
  } else if (f.f == FF) {
    if (f.argc != 0) { paramsError(0); return; }
    UART_printf("Format not supported at this time...");
//    UART_printf("Formatting SD Card..."); nl();
//    f_mkfs(); 
//    UART_printf("Done!"); nl();
  } else if (f.f == FM) { 
    if (f.argc != 0) { paramsError(0); return; }
    UART_printf("Mounting disk... ");
    Fresult = f_mount(&g_sFatFs, "", 0);
    if(Fresult){UART_printf("error"); nl();} 
    else{UART_printf("done"); nl();}
  } else if (f.f == FR) { // Read file      // FF, FR, FW, FD, FL, FC
    UINT successfulreads = 0;
    if (f.argc != 1) { paramsError(1); return; }
    FIL fp;
    f_chdir(currentPath);
    Fresult = f_open(&fp, f.argv[0], FA_READ);
    if (Fresult) {
      UART_printf("There was a problem opening up %s", f.argv[0]); nl();
      UART_printf("Error code: %d", Fresult); nl();
      return; 
    }
    char c;
    do {
      Fresult = f_read(&fp, &c, 1, &successfulreads);
      if (Fresult == FR_OK) {
        if (successfulreads == 1) {
          UART_OutChar(c);
        }
      } else {
        UART_OutString("There was a read problem."); nl();
        f_close(&fp);
        return;
      }
    } while(successfulreads == 1);
    
    f_close(&fp);
    
    nl();
  } else if (f.f == FW) {     // Append to a file
    if (f.argc < 2) { paramsError(2); return; }
    FIL fp; UINT successfulwrites = 0;
    f_chdir(currentPath);
    if (f_open(&fp, f.argv[0], FA_READ|FA_WRITE)) {
      UART_printf("There was a problem opening up %s", f.argv[0]); nl();
      return; 
    }
    int i = 1;
    
    f_lseek(&fp, f_size(&fp));  // seek to the end
    
    char * charPtr;
    for (i = 1; i < f.argc - 1; ++i) {
      char space = ' '; 
      charPtr = f.argv[i];
      while (*charPtr) {
        f_write(&fp, charPtr, 1, &successfulwrites);    // individually write each char
        charPtr++;
      }
      f_write(&fp, &space, 1, &successfulwrites); // Write in a space
      UART_printf("%s ", f.argv[i]);
    }
    charPtr = f.argv[i];
    while (*charPtr) {
      f_write(&fp, charPtr, 1, &successfulwrites);    // individually write each char
      charPtr++;
    }
    UART_printf("%s", f.argv[i]);
    nl();
    f_close(&fp);
  } else if (f.f == FD) {       // Delete a file
    if (f.argc < 1) { paramsError(1); return; }
    f_chdir(currentPath);
    if (f_unlink(f.argv[0])) {
      UART_printf("There was a problem deleting %s", f.argv[0]); nl();
    }
  } else if (f.f == FC) {       // Clear a file
    if (f.argc < 1) { paramsError(1); return; }
    FIL fp;
    f_chdir(currentPath);
    if (f_open(&fp, f.argv[0], FA_READ|FA_WRITE)) {
      UART_printf("There was a problem opening up %s", f.argv[0]); nl();
      return; 
    }
    f_truncate(&fp);
    f_close(&fp);
  } else if (f.f == FN) {     // Create a new file
    if (f.argc != 1) { paramsError(1); return; }
    FIL fp;
    f_chdir(currentPath);
    Fresult = f_open(&fp, f.argv[0], FA_CREATE_ALWAYS|FA_WRITE);
    if (Fresult) {
      UART_printf("There was a problem opening up %s", f.argv[0]); nl();
      return; 
    }
    Fresult = f_close(&fp);
    if (Fresult) {
      UART_printf("There was a problem creating %s", f.argv[0]); nl(); return; 
    }
  } else if (f.f == LS) {   // Print all
    if (f.argc != 0) { paramsError(0); return; }
    DIR dir; FILINFO fno; char * fn;
    int Fresult;
    f_chdir(currentPath);
    f_getcwd(path, PATH_SIZE);
    Fresult = f_opendir(&dir, path); 
    
    if (Fresult == FR_OK) {
      int l = strlen(path);
      for (;;) {
        Fresult = f_readdir(&dir, &fno);                  // read a directory item
        if (Fresult != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir
        if (fno.fname[0] == '.') continue;                 // skip dot entry
        fn = fno.fname;
        if (fno.fattrib & AM_DIR) {
          UART_printf("%s%s/", path, fn); nl();
        } else {
          if (strcmp(path, "/") == 0) { 
            UART_printf("%s%s", path, fn); nl();
          } else {
            UART_printf("%s/%s", path, fn); nl();
          }
        }
      }
    }
  } else if (f.f == MKD) {
    if (f.argc != 1) { paramsError(1); return; }
    int Fresult = f_mkdir(f.argv[0]); 
    if (Fresult) { 
      UART_printf("Error creating directory %s", f.argv[0]); nl();
    }
  } else if (f.f == CD) {
    if (f.argc != 1) { paramsError(1); return; }
    int Fresult = f_chdir(f.argv[1]);
    if (Fresult) {
      UART_printf("Error in changing directory. Error code: %d", Fresult); nl(); 
    } else {
      sprintf(currentPath, "%s", f.argv[0]);
    }
  } else if (f.f == PWD) {
    f_getcwd(path, PATH_SIZE);
    f_chdir(currentPath);
    UART_OutString(path); nl();
  } else if (f.f == DP) {       // ping))) tester. 1 param expected (which ping)
//    if (f.argc != 1) { paramsError(1); return; }
//    long data[5];
//    
////    if (args[0] < 1 || args[0] > 4) {
//    if (args[0] != 4) {
//      UART_printf("Invalid parameter. Expected (1-5)"); nl();
//      return;
//    }
//    UART_printf("Last 5 results for ping))) %d", args[0]); nl();
//    
//    for (int i = 0; i < 5; ++i) {
//      UART_printf("%d", data[i]); nl();
//    }
  } 
}


// ******** Interpreter_Get *********
// Waits for the interpreter to read a command
void Interpreter_Get(void) {
	char buf[BUF_SIZE] = { 0 };
	int err = 1;
	while (err) {
		FunctionInfo f;
		buf[0] = 0;
    UART_printf("> ");
		UART_InString(buf, BUF_SIZE); nl();
		f = parse(buf, &err);
		if (!err) { runFunction(f); }
	}
}

