#ifndef INTERRUPTIONS_H_
#define INTERRUPTIONS_H_

#include "driverlib.h"
#include "device.h"
#include "utils.h" 
extern volatile uint16_t epwmCMPA;

// Protótipos das funções de interrupção
__interrupt void isr_adc_a1(void);
__interrupt void isr_adc_b1(void);

#endif /* INTERRUPTIONS_H_ */
