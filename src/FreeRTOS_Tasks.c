
#include "FreeRTOS_Tasks.h"

StaticTask_t Task1Buffer, Task2Buffer;
StackType_t  Task1Stack[STACK_SIZE], Task2Stack[STACK_SIZE];

StaticTask_t TaskIdleBuffer;
StackType_t  TaskIdleStack[STACK_SIZE];


SemaphoreHandle_t Semaphore = NULL;

QueueHandle_t xQueue_Theta;

UBaseType_t uxHighWaterMark;

uint16_t cnt1 =0, cnt2 = 0;
float sine = 0;

void FreeRTOS_Task1(void * pvParameters){
    static float theta = 3.0;
    while(1){
        if(xSemaphoreTake(Semaphore, 1500) == pdTRUE){

            //PHERIPHERAL VARIABLE
            xSemaphoreGive(Semaphore);
        }

        theta += 6.2831853072/400.0;
        if(theta > 6.2831853071 ) theta -= 6.2831853071;
        else if(theta < -6.2831853071 ) theta += 6.2831853071;

        xQueueOverwrite(xQueue_Theta, (void *)&theta);
        cnt1++;
        //vTaskDelay(5 / portTICK_PERIOD_MS);
        //uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }
}

void FreeRTOS_Task2(void * pvParameters){
    float theta = 0;
    while(1){
        GPIO_togglePin(34);
        //GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
        cnt2++;
        xQueueReceive(xQueue_Theta, (void *)&theta, 5);
        sine = __sin(theta);

        vTaskDelay(50 / portTICK_PERIOD_MS);
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void FreeRTOS_Setup(void){
    StaticSemaphore_t xSemaphoreBuffer;
    uint8_t ucQueueStorageArea[1 * sizeof(float)];
    static StaticQueue_t xStaticQueue;

    Semaphore = xSemaphoreCreateMutexStatic(&xSemaphoreBuffer);
    //xSemaphore = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer);
    xSemaphoreGive(Semaphore);

    xQueue_Theta = xQueueCreateStatic(1,sizeof(float),ucQueueStorageArea,&xStaticQueue);

    // Create the task without using any dynamic memory allocation.
    xTaskCreateStatic(FreeRTOS_Task1,       // Function that implements the task.
                      "Task1",             // Text name for the task.
                      STACK_SIZE,           // Number of indexes in the xStack array.
                      (void *) NULL,        // Parameter passed into the task.
                      tskIDLE_PRIORITY + 1, // Priority at which the task is created.
                      Task1Stack,
                      &Task1Buffer);        // Variable to hold the task's data structure.

    xTaskCreateStatic(FreeRTOS_Task2,       // Function that implements the task.
                      "Task2",             // Text name for the task.
                      STACK_SIZE,           // Number of indexes in the xStack array.
                      (void *) NULL,        // Parameter passed into the task.
                      tskIDLE_PRIORITY + 2, // Priority at which the task is created.
                      Task2Stack,
                      &Task2Buffer);        // Variable to hold the task's data structure.

    vTaskStartScheduler();
}

// ===================== API ================================
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer,
                                     StackType_t *puxIdleTaskStackSize){
   *ppxIdleTaskTCBBuffer = &TaskIdleBuffer;
    *ppxIdleTaskStackBuffer = TaskIdleStack;
    *puxIdleTaskStackSize = STACK_SIZE;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ){
    while(1);
}

