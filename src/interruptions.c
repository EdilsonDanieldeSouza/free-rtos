#include "interruptions.h"



uint16_t adcAResults[RESULTS_BUFFER_A_SIZE] = {0};
uint16_t signalTable[N_SAMPLES] = {0}; 
uint16_t volatile adcIndex = 0;
uint16_t volatile signalIndex = 0; 
uint16_t  result = 0;  

float vout = 0.0f;

float erro = 0.0f;
float erro_acumulado = 0.0f;
float controle_pi = 0.0f;
uint16_t duty_pwm = 0;

#define KP   1.5f
#define KI   0.5f

#define PWM_MAX_LIMIT  3333.0f  
#define PWM_MIN_LIMIT  0.0f

// --- Parâmetros de Controle (Ganhos e Limites) ---
float Kp_Vo = 0.0056256 ;        // Ganho Proporcional original
float Ki_Vo =  30.6271;        // Ganho Integral original
float Kpd_Vo = 0.0f;       // Ganho Proporcional discreto/ajustado
float Kid_Vo = 0.0f;       // Ganho Integral discreto/ajustado
float Umax_Vo = 0.9f;      // Limite máximo de saturação da saída
float Umin_Vo = 0.0f;      // Limite mínimo de saturação da saída

// --- Sinais do Sistema (Entradas e Saídas) ---
float REF = 15.0f;          // Referência (Set-point)
float iof = 0.0f;          // Variável de feedback (Corrente/Tensão medida)
float ek_Vo = 0.0f;        // Erro atual (REF - iof)
float U_Vo = 0.0f;         // Saída final controlada e saturada

// --- Estados Internos do Controlador ---
float uP_Vo = 0.0f;        // Parcela Proporcional atual
float uIk_Vo = 0.0f;       // Parcela Integral atual
float uIk1_Vo = 0.0f;      // Parcela Integral no instante anterior (atraso z^-1)
float v1_Vo = 0.0f;        // Saída do PI antes da saturação (Sinal de controle bruto)
float w1_Vo = 1.0f; // Fator de anti-windup (inicializado em 1.0)

// --- Parâmetro do Sistema ---
float f = 15000;            // Frequência de amostragem ou fator de divisão

__interrupt void isr_adc_a1(void)
{
    
    result = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);  
    adcAResults[adcIndex++] = result;

    // typhoon 
    vout = (float)result * 0.004943320283620 - 0.005206403077689;

    
    
    // Cálculo dos ganhos discretos
    Kpd_Vo = Kp_Vo;
    Kid_Vo = __divf32(Ki_Vo, f);

    // Cálculo do erro
    //ek_Vo = REF - vout;
    ek_Vo = signalTable[signalIndex] - vout;

    // Execução do controle PI
    uP_Vo   = Kpd_Vo * ek_Vo;
    uIk_Vo  = Kid_Vo * (w1_Vo * ek_Vo) + uIk1_Vo; // Parcela integral com Anti-Windup
    uIk1_Vo = uIk_Vo;                             // Atualiza o histórico para o próximo ciclo
    v1_Vo   = uP_Vo + uIk_Vo;                     // Sinal de controle total (bruto)

    // Lógica de Saturação e Anti-Windup dinâmico
    if (v1_Vo > Umax_Vo) {
        U_Vo  = Umax_Vo;
        w1_Vo = 0.0f; // Congela a integração se estourar o limite superior
    }
    else if (v1_Vo < Umin_Vo) {
        U_Vo  = Umin_Vo;
        w1_Vo = 0.0f; // Congela a integração se estourar o limite inferior
    }
    else {
        U_Vo  = v1_Vo;
        w1_Vo = 1.0f; // Integração normal
    }
    
    duty_pwm = 3333 * U_Vo;
    //EPWM_setCounterCompareValue(EPWM7_BASE, EPWM_COUNTER_COMPARE_A, signalTable[signalIndex++]);
    EPWM_setCounterCompareValue(EPWM2_BASE, EPWM_COUNTER_COMPARE_A, duty_pwm);
    signalIndex++;
    if(adcIndex >= RESULTS_BUFFER_A_SIZE)
    {
        adcIndex = 0;
    }
    
    if(signalIndex >= N_SAMPLES)
    {
        signalIndex = 0; 
    }
    
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    
    if(true == ADC_getInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

__interrupt void isr_adc_b1(void)
{
    // a ser implementada no futuro
     // usar tabela  em tensão
    //er = result - signalTable[signalIndex++] * 3 / 4095;
    
    // usar tabela ja desenhada para a leitura do ad
   /* erro =  (float)signalTable[signalIndex] - (float)result;
    erro_acumulado += erro;

    if(erro_acumulado > 3333.0f)  erro_acumulado = 3333.0f;
    if(erro_acumulado < -3333.0f) erro_acumulado = -3333.0f;


    controle_pi = (KP * erro) + (KI * erro_acumulado);

    
    if(controle_pi > PWM_MAX_LIMIT) controle_pi = PWM_MAX_LIMIT;
    if(controle_pi < PWM_MIN_LIMIT) controle_pi = PWM_MIN_LIMIT;

    duty_pwm = (uint16_t)controle_pi;*/

    
}
