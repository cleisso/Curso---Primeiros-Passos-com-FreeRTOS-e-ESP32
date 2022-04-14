#include <Arduino.h>

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t semaphoreMutex = NULL;

void vTaskPrint1 (void *pvParameters);
void vTaskPrint2 (void *pvParameters);
void PrintSerial (const char *message);

void setup()
{
	Serial.begin(9600);

	semaphoreMutex = xSemaphoreCreateMutex();

	if (semaphoreMutex == NULL)
	{
		Serial.println("Nao foi possivel criar o semaforo! :(");
		while (1);
	}

	xTaskCreatePinnedToCore(vTaskPrint1, "Task 1", configMINIMAL_STACK_SIZE + 2000, NULL, 1, NULL, APP_CPU_NUM);
	xTaskCreatePinnedToCore(vTaskPrint2, "Task 2", configMINIMAL_STACK_SIZE + 2000, NULL, 2, NULL, APP_CPU_NUM);
}

void loop()
{
	vTaskDelay(2000);
}

void vTaskPrint1 (void *pvParameters)
{
	(void)pvParameters;

	while (1)
	{
		PrintSerial(pcTaskGetTaskName(NULL));
		vTaskDelay(1);
	}
}

void vTaskPrint2 (void *pvParameters)
{
	(void)pvParameters;

	while (1)
	{
		PrintSerial(pcTaskGetTaskName(NULL));
		vTaskDelay(1);
	}
}

void PrintSerial (const char *message)
{
	char buffer[50];
	char size;

	xSemaphoreTake(semaphoreMutex, portMAX_DELAY);
	size = snprintf(buffer, sizeof(buffer), "Task em execucao: %s\r\n", message);

	for (uint8_t i = 0; i < size; ++i)
	{
		Serial.write(buffer[i]);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	xSemaphoreGive(semaphoreMutex);
	vTaskDelay(1);
}
