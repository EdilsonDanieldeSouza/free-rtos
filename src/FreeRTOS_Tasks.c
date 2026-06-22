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

SemaphoreHandle_t xMutexUART = NULL;
QueueHandle_t xQueue_Theta;
UBaseType_t uxHighWaterMark;

// ===================== TASKS ================================

// envia dados pela uart
void FreeRTOS_Task1(void * pvParameters) {
    uint16_t receivedChar;
    unsigned char *msg;
    uint16_t rxStatus = 0U;
    uint32_t timeout = 0;
    
    //
    // Send starting message.
    //
    msg = "\r\n\n\nHello World!\0";
    
    // Protege acesso à UART com Mutex
    xSemaphoreTake(xMutexUART, portMAX_DELAY);
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 17);
    xSemaphoreGive(xMutexUART);
    
    msg = "\r\nYou will enter a character, and the DSP will echo it back!\n\0";
    xSemaphoreTake(xMutexUART, portMAX_DELAY);
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 62);
    xSemaphoreGive(xMutexUART);

    for(;;)
    {
        msg = "\r\nEnter a character: \0";
        xSemaphoreTake(xMutexUART, portMAX_DELAY);
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 22);
        xSemaphoreGive(xMutexUART);

        //
        // Read a character from the FIFO with timeout.
        // Se nenhum caractere chegar em 1 segundo, pula para próxima iteração
        //
        timeout = 0;
        while(!SCI_isCharAvailable(SCIA_BASE) && timeout < 100)
        {
            vTaskDelay(pdMS_TO_TICKS(10));  // Permite outras tasks executarem
            timeout++;
        }

        // Se timeout expirou, pula para próxima iteração
        if(timeout >= 100)
        {
            continue;
        }

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
        xSemaphoreTake(xMutexUART, portMAX_DELAY);
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 13);
        SCI_writeCharBlockingFIFO(SCIA_BASE, receivedChar);
        xSemaphoreGive(xMutexUART);

        //
        // Increment the loop count variable.
        //
        loopCounter++;
    }
}

// Task2 - Pisca LED e envia mensagem pela UART
void FreeRTOS_Task2(void * pvParameters) {
    // Configura a taxa de atualização (ex: 250ms - pisca mais rápido)
    const TickType_t xDelay = pdMS_TO_TICKS(250);
    uint32_t j = 0;  // INICIALIZADO CORRETAMENTE
    uint32_t messageCounter = 0;  // Contador de mensagens
    unsigned char *msg;
    
    for( ;; ) {
        if(j >= 40000000){ 
            j = 0;
        }
        else{
            j++;
        }
        
        // Exemplo de função DriverLib para alternar o estado do pino
        GPIO_togglePin(LED_BLUE_GPIO);
        
        // Envia mensagem pela UART a cada 1 segundo (4 × 250ms)
        messageCounter++;
        if(messageCounter >= 4)
        {
            messageCounter = 0;
            
            // Protege acesso à UART com Mutex
            xSemaphoreTake(xMutexUART, portMAX_DELAY);
            msg = "\r\n[Task2] LED toggled - Message from Task 2!\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 46);
            xSemaphoreGive(xMutexUART);
        }
        
        // Bloqueia a task pelo tempo determinado
        vTaskDelay(xDelay);
    }
}

// ===================== SETUP ================================

void FreeRTOS_Setup(void){
    // CORREÇÃO: Adicionado 'static' para garantir que os buffers persistam na memória
    static StaticSemaphore_t xMutexBuffer;
    static uint8_t ucQueueStorageArea[1 * sizeof(float)];
    static StaticQueue_t xStaticQueue;

    // Criação do Mutex Estático
    xMutexUART = xSemaphoreCreateMutexStatic(&xMutexBuffer);
    
    // Nota: Mutexes já são criados "dados" (disponíveis). 
    // Você só precisa de xSemaphoreGive se estivesse usando um Semáforo Binário.
    // xSemaphoreGive(xMutexUART); 

    // Criação da Fila Estática
    xQueue_Theta = xQueueCreateStatic(1, sizeof(float), ucQueueStorageArea, &xStaticQueue);

    // Criação das Tasks sem alocação dinâmica
    xTaskCreateStatic(FreeRTOS_Task1, 
                      "Task1", 
                      STACK_SIZE, 
                      (void *) NULL, 
                      tskIDLE_PRIORITY + 2,  // Task 1 com prioridade média-alta (para UART)
                      Task1Stack,
                      &Task1Buffer); 

    xTaskCreateStatic(FreeRTOS_Task2, 
                      "Task2", 
                      STACK_SIZE, 
                      (void *) NULL, 
                      tskIDLE_PRIORITY + 1,  // Task 2 com prioridade média (LED não é crítico)
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
