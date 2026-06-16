#include "utils.h"    

void createTable(void)
{
    uint16_t i;

    // Marcos de tempo acumulados para os 20 ms (300 amostras a 15 kHz)
    const uint16_t fim_subida  = 15;                // 1 ms (Amostras 0 a 14)
    const uint16_t fim_alto    = 15 + 135;          // 9 ms (Amostras 15 a 149) -> Total: 150
    const uint16_t fim_descida = 15 + 135 + 15;     // 1 ms (Amostras 150 a 164) -> Total: 165
    
    // Duração real da rampa (usada como divisor para evitar divisões por zero)
    const uint16_t duracao_rampa = 15; 

    for (i = 0; i < N_SAMPLES; i++)
    {
        if (i < fim_subida)
        {
            // 1. Rampa de subida: vai de 101 até 303 ao longo de 15 amostras
            signalTable[i] = CMPA_LOW + 
                (uint16_t)(((uint32_t)(CMPA_HIGH - CMPA_LOW) * i) / duracao_rampa);
        }
        else if (i < fim_alto)
        {
            // 2. Nível alto sustentado em 18V (303) por 135 amostras
            signalTable[i] = CMPA_HIGH;
        }
        else if (i < fim_descida)
        {
            // 3. Rampa de descida: vai de 303 até 101 ao longo de 15 amostras
            // i - fim_alto descobre quantos passos já demos dentro da rampa de descida
            signalTable[i] = CMPA_HIGH - 
                (uint16_t)(((uint32_t)(CMPA_HIGH - CMPA_LOW) * (i - fim_alto)) / duracao_rampa);
        }
        else
        {
            // 4. Nível baixo sustentado em 6V (101) pelo restante do ciclo (135 amostras)
            signalTable[i] = CMPA_LOW;
        }
    }
}
