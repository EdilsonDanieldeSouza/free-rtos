#ifndef PERIPHERAL_SETUP_H_
#define PERIPHERAL_SETUP_H_

#include "driverlib.h"
#include "device.h"
#define ADC_RESOLUTION  12
#define LED_RED_GPIO   DEVICE_GPIO_PIN_LED1  
#define LED_BLUE_GPIO  DEVICE_GPIO_PIN_LED2  


// Protótipos das funções de inicialização
void configLEDS(void);
void configUART(void);
void configEPWM1(void);
void configEPWM2(void);
void configEPWM7(void);
void initADC(void);
void initADCSOC(void);


#endif /* PERIPHERAL_SETUP_H_ */
