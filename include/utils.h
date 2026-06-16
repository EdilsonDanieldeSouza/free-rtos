#ifndef UTILS_H_
#define UTILS_H_

#include "driverlib.h"
#include "device.h"
   
#define RESULTS_BUFFER_A_SIZE   300u  
#define RESULTS_BUFFER_B_SIZE   300u  
#define N_SAMPLES               300u  

// definicoes para tabela desenhada para o ad
//#define CMPA_HIGH               2321u  
//#define CMPA_LOW                1229u 

// definicoes para tabela em tensao
#define CMPA_HIGH               18u  
#define CMPA_LOW                6u 

extern uint16_t adcAResults[RESULTS_BUFFER_A_SIZE];              
extern uint16_t signalTable[N_SAMPLES];     
extern volatile uint16_t signalIndex; 
extern volatile uint16_t adcIndex; 

void createTable(void);

#endif
