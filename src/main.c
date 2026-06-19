#include "driverlib.h"
#include "device.h"
#include "Peripheral_Setup.h"
#include "utils.h"
#include "FreeRTOS_Tasks.h"
uint32_t i = 0;
uint32_t j = 0;
void main(void)
{

    Device_init();
    Device_initGPIO();
    
    Interrupt_initModule();
    Interrupt_initVectorTable();

    configLEDS();
    configUART();
    Interrupt_enable(INT_ADCA1);             
    EINT;
    ERTM;    
           
    FreeRTOS_Setup();
    while(1)
    {
       
    }
}
