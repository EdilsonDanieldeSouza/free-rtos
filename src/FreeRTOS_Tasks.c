#include "FreeRTOS_Tasks.h"
#include "driverlib.h"
#include "device.h"

uint16_t loopCounter = 0;

#define LED_RED_GPIO   DEVICE_GPIO_PIN_LED1  
#define LED_BLUE_GPIO  DEVICE_GPIO_PIN_LED2  

StaticTask_t Task1Buffer, Task2Buffer;
StackType_t  Task1Stack[STACK_SIZE], Task2Stack[STACK_SIZE];

StaticTask_t TaskIdleBuffer;
StackType_t  TaskIdleStack[STACK_SIZE];

SemaphoreHandle_t Semaphore = NULL;
QueueHandle_t xQueue_Theta;
UBaseType_t uxHighWaterMark;

// ===================== TASKS ================================

// envia dados pela uart
void FreeRTOS_Task1(void * pvParameters) {
    uint16_t receivedChar;
    unsigned char *msg;
    uint16_t rxStatus = 0U;
    //
    // Send starting message.
    //
    msg = "\r\n\n\nHello World!\0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 17);
    msg = "\r\nYou will enter a character, and the DSP will echo it back!\n\0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 62);

    for(;;)
    {
        msg = "\r\nEnter a character: \0";
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 22);

        //
        // Read a character from the FIFO.
        //
        receivedChar = SCI_readCharBlockingFIFO(SCIA_BASE);

        rxStatus = SCI_getRxStatus(SCIA_BASE);
        if((rxStatus & SCI_RXSTATUS_ERROR) != 0)
        {
            //
            //If Execution stops here there is some error
            //Analyze SCI_getRxStatus() API return value
            //
           // ESTOP0;
        }

        //
        // Echo back the character.
        //
        msg = "  You sent: \0";
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 13);
        SCI_writeCharBlockingFIFO(SCIA_BASE, receivedChar);

        //
        // Increment the loop count variable.
        //
        loopCounter++;
    }
}

// Piscar LED Azul
void FreeRTOS_Task2(void * pvParameters) {
    // Configura a taxa de atualização (ex: 250ms - pisca mais rápido)
    const TickType_t xDelay = pdMS_TO_TICKS(250);
    uint32_t j;
    
    for( ;; ) {
        if(j>= 40000000){ j=0;}else{j++;}
        // Exemplo de função DriverLib para alternar o estado do pino
        GPIO_togglePin(LED_BLUE_GPIO);
        
        // Bloqueia a task pelo tempo determinado
        vTaskDelay(xDelay);
    }
}

// ===================== SETUP ================================

void FreeRTOS_Setup(void){
    // CORREÇÃO: Adicionado 'static' para garantir que os buffers persistam na memória
    static StaticSemaphore_t xSemaphoreBuffer;
    static uint8_t ucQueueStorageArea[1 * sizeof(float)];
    static StaticQueue_t xStaticQueue;

    // Criação do Mutex Estático
    Semaphore = xSemaphoreCreateMutexStatic(&xSemaphoreBuffer);
    
    // Nota: Mutexes já são criados "dados" (disponíveis). 
    // Você só precisa de xSemaphoreGive se estivesse usando um Semáforo Binário.
    // xSemaphoreGive(Semaphore); 

    // Criação da Fila Estática
    xQueue_Theta = xQueueCreateStatic(1, sizeof(float), ucQueueStorageArea, &xStaticQueue);

    // Criação das Tasks sem alocação dinâmica
    xTaskCreateStatic(FreeRTOS_Task1, 
                      "Task1", 
                      STACK_SIZE, 
                      (void *) NULL, 
                      tskIDLE_PRIORITY + 1, 
                      Task1Stack,
                      &Task1Buffer); 

    xTaskCreateStatic(FreeRTOS_Task2, 
                      "Task2", 
                      STACK_SIZE, 
                      (void *) NULL, 
                      tskIDLE_PRIORITY + 2, // Task 2 tem maior prioridade
                      Task2Stack,
                      &Task2Buffer); 

    // Inicia o Scheduler
    vTaskStartScheduler();
}

// ===================== API / HOOKS ================================

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer, 
                                    StackType_t *puxIdleTaskStackSize){
    *ppxIdleTaskTCBBuffer = &TaskIdleBuffer;
    *ppxIdleTaskStackBuffer = TaskIdleStack;
    *puxIdleTaskStackSize = STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){
    // Se o código parar aqui, aumente o valor de STACK_SIZE
    while(1);
}
