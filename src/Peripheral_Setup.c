#include "Peripheral_Setup.h"

void configLEDS(void){
    GPIO_setPadConfig(LED_RED_GPIO, GPIO_PIN_TYPE_STD); // Standard pad
    GPIO_setPinConfig(LED_RED_GPIO); // Standard push-pull
    GPIO_setDirectionMode(LED_RED_GPIO, GPIO_DIR_MODE_OUT); // SET AS OUTPUT

    GPIO_setPadConfig(LED_BLUE_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(LED_BLUE_GPIO);
    GPIO_setDirectionMode(LED_BLUE_GPIO, GPIO_DIR_MODE_OUT);
}

void configUART(void)
{
    //
    // Configuration for the SCI Rx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // Configuration for the SCI Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //
    // Initialize interrupt controller and vector table.
    //
    Interrupt_initModule();
    Interrupt_initVectorTable();

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);

    //
    // Configure SCIA for echoback.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 9600, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

#ifdef AUTOBAUD
    //
    // Perform an autobaud lock.
    // SCI expects an 'a' or 'A' to lock the baud rate.
    //
    SCI_lockAutobaud(SCIA_BASE);
#endif


    
}


// ==========================================================
// 1. ePWM1 Configuration (Gera PWM a 7.5 khz  e Dispara ADC a 15kHz), pwm só é usado para acionamento do adc
// ==========================================================
void configEPWM1(void)
{
    // Trava o sincronismo para configurar os registradores de tempo de forma limpa
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    
    EALLOW;
    // --- Submódulo Time-Base (TB) ---
    // Período para 15kHz em modo Up-Down: (100MHz /  2 * 7.5k) = 6666
    EPWM_setTimeBasePeriod(EPWM1_BASE, 3333); 
    EPWM_setPhaseShift(EPWM1_BASE, 0);
    EPWM_setTimeBaseCounter(EPWM1_BASE, 0);
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM1_BASE);
    EPWM_setClockPrescaler(EPWM1_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setSyncOutPulseMode(EPWM1_BASE, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);

    // --- Submódulo Counter-Compare (CC) ---
    // Duty cycle inicial em 50% 
    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, 3333);
    EPWM_setCounterCompareShadowLoadMode(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);
    
    // --- Submódulo Action-Qualifier (AQ) ---
    EPWM_setActionQualifierAction(EPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    EDIS;
    // --- Configuração do Gatilho do ADCA (SOCA) ---
    EPWM_disableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
    
    // Dispara o ADC quando o contador estiver subindo e igualar ao CMPA
    EPWM_setADCTriggerSource(EPWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_ZERO_OR_PERIOD);
    EPWM_setADCTriggerEventPrescale(EPWM1_BASE, EPWM_SOC_A, 1);
    EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_A); // Habilita o disparo de fato

     // --- Configuração do Gatilho do ADCB (SOCB) ---
    EPWM_disableADCTrigger(EPWM1_BASE, EPWM_SOC_B);
    
    // Dispara o ADC quando o contador estiver subindo e igualar ao CMPA
    EPWM_setADCTriggerSource(EPWM1_BASE, EPWM_SOC_B, EPWM_SOC_TBCTR_ZERO_OR_PERIOD);
    EPWM_setADCTriggerEventPrescale(EPWM1_BASE, EPWM_SOC_B, 1);
    EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_B); // Habilita o disparo de fato

    // Libera o relógio de sincronismo
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

// ==========================================================
// 2. ePWM2 Configuration (Gera PWM para controlar o buck f=150khz)
// ==========================================================
void configEPWM2(void)
{
    EALLOW;
    GPIO_setPadConfig(2, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_2_EPWM2A);
    EDIS;
    
    // Trava o sincronismo para configurar os registradores de tempo de forma limpa
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    EALLOW;
    // --- Submódulo Time-Base (TB) ---
    // Período para 150kHz em modo Up-Down: (100MHz /  2 * 15k) = 3333
    EPWM_setTimeBasePeriod(EPWM2_BASE, 3333); 
    EPWM_setPhaseShift(EPWM2_BASE, 0);
    EPWM_setTimeBaseCounter(EPWM2_BASE, 0);
    EPWM_setTimeBaseCounterMode(EPWM2_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM2_BASE);
    EPWM_setClockPrescaler(EPWM2_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setSyncOutPulseMode(EPWM2_BASE, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);


    // --- Submódulo Counter-Compare (CC) ---
    // Duty cycle inicial em 50% (Metade de 3333 é  1666)
    EPWM_setCounterCompareValue(EPWM2_BASE, EPWM_COUNTER_COMPARE_A, 1666);
    EPWM_setCounterCompareShadowLoadMode(EPWM2_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);
    
    // --- Submódulo Action-Qualifier (AQ) ---
    EPWM_setActionQualifierAction(EPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    EDIS;

    // Libera o relógio de sincronismo
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}



void configEPWM7(void)
{
    EALLOW;
    GPIO_setPadConfig(157, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_157_EPWM7A);
    EDIS;
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Configura o PWM físico em 150 kHz
    EPWM_setTimeBasePeriod(EPWM7_BASE, 333); 
    EPWM_setPhaseShift(EPWM7_BASE, 0);
    EPWM_setTimeBaseCounter(EPWM7_BASE, 0);
    EPWM_setTimeBaseCounterMode(EPWM7_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM7_BASE);
    EPWM_setClockPrescaler(EPWM7_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);

    // Carrega o Duty Cycle no início/fim para evitar glitches
    EALLOW;
    EPWM_setCounterCompareShadowLoadMode(EPWM7_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);

    // Configura a lógica do pino (Sobe na subida do CMPA, desce na descida)
    EPWM_setActionQualifierAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EDIS;
    // Libera o relógio de sincronismo
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}


//
// Function to configure and power up ADCA.
//
void initADC(void){
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ADCA);
    //
    // Set ADCDLK divider to /4
    //
    ADC_setPrescaler(ADCA_BASE, ADC_CLK_DIV_4_0);

    //
    // Set resolution and signal mode (see #defines above) and load
    // corresponding trims.
    //
#if(ADC_RESOLUTION == 12)
    ADC_setMode(ADCA_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
#elif(EX_ADC_RESOLUTION == 16)
    ADC_setMode(ADCA_BASE, ADC_RESOLUTION_16BIT, ADC_MODE_DIFFERENTIAL);
#endif

    //
    // Set pulse positions to late
    //
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);

    //
    // Power up the ADC and then delay for 1 ms
    //
    ADC_enableConverter(ADCA_BASE);
    DEVICE_DELAY_US(1000);
}

//
// Function to configure ADCA's SOC0 to be triggered by ePWM1.
//
void initADCSOC(void)
{

#if(ADC_RESOLUTION == 12)
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM1_SOCA,
                 ADC_CH_ADCIN4, 15);
#elif(EX_ADC_RESOLUTION == 16)
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM1_SOCA,
                 ADC_CH_ADCIN4, 64);
#endif

    //
    // Set SOC0 to set the interrupt 1 flag. Enable the interrupt and make
    // sure its flag is cleared.
    //
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
}
