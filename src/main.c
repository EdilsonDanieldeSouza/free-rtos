#include "driverlib.h"
#include "device.h"
#include "Peripheral_Setup.h"
#include "interruptions.h"
#include "utils.h"

volatile uint16_t epwmCMPA = 0.5;

void main(void)
{
    Device_init();
    Device_initGPIO();
    
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_ADCA1, &isr_adc_a1);

    createTable();     
    initADC();       
    initADCSOC();      
    configEPWM1();     
    configEPWM2();     
    configEPWM7();     


    Interrupt_enable(INT_ADCA1);             
    EINT;
    ERTM;           

    while(1)
    {
       
    }
}
