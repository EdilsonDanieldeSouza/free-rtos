#ifndef FREERTOS_TASKS_H_
#define FREERTOS_TASKS_H_

#include "Peripheral_Setup.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define STACK_SIZE (1024u)

void FreeRTOS_Setup(void);

void FreeRTOS_Task1(void *pvParameters);
void FreeRTOS_Task2(void *pvParameters);

#endif 
