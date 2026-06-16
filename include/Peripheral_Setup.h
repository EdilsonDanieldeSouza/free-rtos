#ifndef PERIPHERAL_SETUP_H_
#define PERIPHERAL_SETUP_H_

#include "driverlib.h"
#include "device.h"
#define ADC_RESOLUTION  12



// Protótipos das funções de inicialização

void configEPWM1(void);
void configEPWM2(void);
void configEPWM7(void);
void initADC(void);
void initADCSOC(void);


#endif /* PERIPHERAL_SETUP_H_ */
